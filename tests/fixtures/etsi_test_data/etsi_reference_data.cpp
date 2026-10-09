#include "etsi_reference_data.h"

namespace eti::test {

// Simple reference ETI frame with valid structure
const std::array<uint8_t, 6144> ETSI_REFERENCE_ETI_FRAME = {{
    // ETI Frame header (simplified)
    0x49, 0x4E, 0x49, 0x54,  // "INIT" - ETI sync pattern
    0x00, 0x00, 0x00, 0x01,  // Frame counter
    // Fill rest with test pattern
    0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
    0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F,
    // Initialize remaining bytes to zero for simplicity
    // (In a real implementation, this would contain valid ETI frame data)
}};

// Reference services for testing
const std::vector<ETSIReferenceService> ETSI_REFERENCE_SERVICES = {
    {0x1001, "Test Service 1", true, 0, 0, 64},
    {0x1002, "Test Service 2", false, 1, 64, 96},
    {0x1003, "Test Service 3", true, 2, 160, 128}
};

// Test network configuration
const char* ETSI_TEST_MULTICAST_ADDRESS = "239.192.0.1";
const uint16_t ETSI_TEST_MULTICAST_PORT = 9200;

} // namespace eti::test