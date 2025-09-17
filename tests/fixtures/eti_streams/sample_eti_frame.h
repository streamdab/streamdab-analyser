#pragma once

#include <cstdint>
#include <vector>

/**
 * @file sample_eti_frame.h
 * @brief Sample ETI frame data for TDD testing
 * 
 * This file contains sample ETI (Ensemble Transport Interface) frame data
 * compliant with ETSI EN 300 799 for testing purposes.
 */

namespace EtiTestData {

/**
 * @brief Standard ETI frame structure (6144 bytes)
 * 
 * ETI Frame Structure (ETSI EN 300 799):
 * - Frame Synchronization (4 bytes): 0xFF 0xFF 0xFF 0xFF
 * - Frame Header (8 bytes)
 * - Sub-channel Data (varies)
 * - EOF (End of Frame) + CRC (4 bytes)
 */
struct EtiFrame {
    static constexpr size_t FRAME_SIZE = 6144;
    static constexpr size_t SYNC_SIZE = 4;
    static constexpr size_t HEADER_SIZE = 8;
    static constexpr size_t EOF_CRC_SIZE = 4;
    
    uint8_t data[FRAME_SIZE];
    
    EtiFrame();
    bool isValid() const;
};

/**
 * @brief Sample compliant ETI frame for TDD RED phase testing
 * 
 * This frame should initially FAIL processing until implementation exists
 */
extern const EtiFrame SAMPLE_COMPLIANT_FRAME;

/**
 * @brief Sample corrupted ETI frame for error handling tests
 * 
 * This frame has intentional corruption for negative testing
 */
extern const EtiFrame SAMPLE_CORRUPTED_FRAME;

/**
 * @brief Multi-service ETI frame with multiple audio services
 * 
 * Contains multiple DAB/DAB+ audio services for comprehensive testing
 */
extern const EtiFrame MULTI_SERVICE_FRAME;

/**
 * @brief Empty ETI frame template
 * 
 * Basic frame structure with no services for minimal testing
 */
extern const EtiFrame EMPTY_FRAME;

/**
 * @brief Performance test ETI stream (1000 frames)
 * 
 * Large dataset for performance and stress testing
 */
class PerformanceTestStream {
public:
    static constexpr size_t FRAME_COUNT = 1000;
    static std::vector<EtiFrame> generate();
    static std::vector<uint8_t> generateRawData();
};

/**
 * @brief FIC (Fast Information Channel) test data
 * 
 * Sample FIC blocks for FIG (Fast Information Group) testing
 */
namespace FicTestData {
    // FIG Type 0/0: Basic ensemble information
    extern const std::vector<uint8_t> FIG_0_0_ENSEMBLE_INFO;
    
    // FIG Type 0/1: Basic sub-channel organization
    extern const std::vector<uint8_t> FIG_0_1_SUBCHANNEL_ORG;
    
    // FIG Type 0/2: Basic service organization
    extern const std::vector<uint8_t> FIG_0_2_SERVICE_ORG;
    
    // FIG Type 1/0: Service labels
    extern const std::vector<uint8_t> FIG_1_0_SERVICE_LABELS;
    
    // Complete FIC block with multiple FIGs
    extern const std::vector<uint8_t> COMPLETE_FIC_BLOCK;
}

/**
 * @brief Audio test data for DAB/DAB+ decoding
 */
namespace AudioTestData {
    // Sample DAB audio frame (MP2)
    extern const std::vector<uint8_t> SAMPLE_DAB_AUDIO_FRAME;
    
    // Sample DAB+ audio frame (AAC)
    extern const std::vector<uint8_t> SAMPLE_DABPLUS_AUDIO_FRAME;
    
    // Corrupted audio frame for error testing
    extern const std::vector<uint8_t> CORRUPTED_AUDIO_FRAME;
}

/**
 * @brief Utility functions for test data generation
 */
class TestDataGenerator {
public:
    /**
     * @brief Generate a valid ETI frame with specified parameters
     * @param frameNumber Frame counter value
     * @param serviceCount Number of services to include
     * @return Generated ETI frame
     */
    static EtiFrame generateValidFrame(uint32_t frameNumber, uint8_t serviceCount = 1);
    
    /**
     * @brief Generate an ETI frame with specific error type
     * @param errorType Type of error to inject
     * @return Corrupted ETI frame
     */
    enum class ErrorType {
        INVALID_SYNC,
        CORRUPTED_HEADER,
        INVALID_CRC,
        TRUNCATED_FRAME,
        OVERSIZED_FRAME
    };
    
    static EtiFrame generateErrorFrame(ErrorType errorType);
    
    /**
     * @brief Generate a stream of ETI frames for testing
     * @param frameCount Number of frames to generate
     * @param withErrors Whether to include error frames
     * @return Vector of ETI frames
     */
    static std::vector<EtiFrame> generateStream(size_t frameCount, bool withErrors = false);
};

} // namespace EtiTestData