#pragma once

#include "../../src/core/eti_types.h"

/**
 * @file network_test_data.h
 * @brief Test data constants for network processing unit tests
 * 
 * This file provides pre-defined network data and test constants for use in
 * unit tests following TDD methodology. These constants enable consistent
 * and reliable testing across all network test suites.
 */

namespace NetworkTestData {

/**
 * @brief Sample ETI-over-IP data for testing
 * 
 * This data contains valid ETI-over-IP structure for testing
 * network streaming and processing functionality.
 */
extern const uint8_t SAMPLE_ETI_OVER_IP_DATA[];
extern const size_t SAMPLE_ETI_OVER_IP_SIZE;

/**
 * @brief Test multicast address for ETI-over-IP
 */
extern const char* const TEST_MULTICAST_ADDRESS;

/**
 * @brief Test port for ETI-over-IP
 */
extern const uint16_t TEST_ETI_PORT;

/**
 * @brief Sample network buffer sizes for testing
 */
extern const size_t TEST_BUFFER_SIZE;
extern const size_t TEST_MAX_PACKET_SIZE;

} // namespace NetworkTestData