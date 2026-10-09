#pragma once

#include "../../src/core/eti_types.h"

/**
 * @file eti_test_data.h
 * @brief Test data constants for ETI processing unit tests
 * 
 * This file provides pre-defined ETI frames and test data for use in
 * unit tests following TDD methodology. These constants enable consistent
 * and reliable testing across all test suites.
 */

namespace EtiTestData {

/**
 * @brief Sample compliant ETI frame for testing
 * 
 * This frame contains valid ETSI-compliant structure with proper
 * synchronization, LIDATA, FIC, and MSC sections for testing
 * successful processing scenarios.
 */
extern const eti::EtiFrame SAMPLE_COMPLIANT_FRAME;

/**
 * @brief Sample corrupted ETI frame for testing
 * 
 * This frame contains intentional corruption for testing error
 * detection and handling scenarios.
 */
extern const eti::EtiFrame SAMPLE_CORRUPTED_FRAME;

/**
 * @brief Test ensemble with multiple services
 * 
 * Provides a realistic ensemble configuration for testing
 * service discovery and management.
 */
extern const eti::EtiFrame MULTI_SERVICE_ENSEMBLE_FRAME;

/**
 * @brief Empty/silent ETI frame for testing
 * 
 * Contains valid structure but no audio data, useful for
 * testing silence detection and handling.
 */
extern const eti::EtiFrame EMPTY_FRAME;

/**
 * @brief FIC test data namespace
 * 
 * Contains specific FIC section test data for detailed
 * Fast Information Channel testing.
 */
namespace FicTestData {
    /**
     * @brief Valid FIC block with ensemble information
     */
    extern const std::array<uint8_t, 32> VALID_FIC_BLOCK;
    
    /**
     * @brief Corrupted FIC block for error testing
     */
    extern const std::array<uint8_t, 32> CORRUPTED_FIC_BLOCK;
    
    /**
     * @brief FIC block with service list
     */
    extern const std::array<uint8_t, 32> SERVICE_LIST_FIC_BLOCK;
}

/**
 * @brief MSC test data namespace
 * 
 * Contains Main Service Channel test data for audio
 * and data service testing.
 */
namespace MscTestData {
    /**
     * @brief DAB audio service data
     */
    extern const std::array<uint8_t, 2048> DAB_AUDIO_SERVICE;
    
    /**
     * @brief DAB+ audio service data
     */
    extern const std::array<uint8_t, 2048> DABPLUS_AUDIO_SERVICE;
    
    /**
     * @brief Data service content
     */
    extern const std::array<uint8_t, 2048> DATA_SERVICE;
}

/**
 * @brief Utility functions for test data generation
 */
namespace TestUtils {
    /**
     * @brief Generate random ETI frame with valid structure
     * @param seed Random seed for reproducible results
     * @return Generated ETI frame
     */
    eti::EtiFrame generateRandomFrame(uint32_t seed = 12345);
    
    /**
     * @brief Create frame with specific service ID
     * @param serviceId Service identifier
     * @return ETI frame configured for specified service
     */
    eti::EtiFrame createFrameWithService(uint32_t serviceId);
    
    /**
     * @brief Corrupt specific frame fields for error testing
     * @param frame Source frame to corrupt
     * @param corruptSync Corrupt synchronization field
     * @param corruptFic Corrupt FIC section
     * @param corruptMsc Corrupt MSC section
     * @return Corrupted frame
     */
    eti::EtiFrame corruptFrame(const eti::EtiFrame& frame, 
                              bool corruptSync = false,
                              bool corruptFic = false, 
                              bool corruptMsc = false);
}

} // namespace EtiTestData