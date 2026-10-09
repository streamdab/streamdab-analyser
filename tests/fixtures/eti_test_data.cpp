#include "eti_test_data.h"
#include <cstring>
#include <array>

/**
 * @file eti_test_data.cpp
 * @brief Implementation of test data constants for ETI processing unit tests
 */

namespace EtiTestData {

// Sample compliant ETI frame implementation
const eti::EtiFrame SAMPLE_COMPLIANT_FRAME = []() {
    eti::EtiFrame frame;
    
    // Initialize with valid ETI frame structure
    // Sync pattern (bytes 0-3)
    frame.frame_data[0] = 0xFF;
    frame.frame_data[1] = 0xFF;
    frame.frame_data[2] = 0xFF;
    frame.frame_data[3] = 0xFF;
    
    // LIDATA section (bytes 4-11)
    frame.frame_data[4] = 0x00;  // FC (Frame Count)
    frame.frame_data[5] = 0x01;  // FICF=0, NST=1
    frame.frame_data[6] = 0x00;  // FP
    frame.frame_data[7] = 0x00;  // MID high
    frame.frame_data[8] = 0x01;  // MID low
    frame.frame_data[9] = 0x10;  // FL (Frame Length)
    frame.frame_data[10] = 0x00; // Reserved
    frame.frame_data[11] = 0x00; // Reserved
    
    // FIC section (bytes 12-43) - 32 bytes
    frame.frame_data[12] = 0x00; // FIG Type 0
    frame.frame_data[13] = 0x00; // Extension 0
    frame.frame_data[14] = 0x12; // Ensemble ID high
    frame.frame_data[15] = 0x34; // Ensemble ID low
    
    // Fill rest of FIC with padding
    for (int i = 16; i < 44; i++) {
        frame.frame_data[i] = 0x00;
    }
    
    // MSC section - fill with test pattern
    for (size_t i = 44; i < frame.frame_data.size() - 4; i++) {
        frame.frame_data[i] = static_cast<uint8_t>(i & 0xFF);
    }
    
    // CRC (last 4 bytes) - simplified
    frame.frame_data[frame.frame_data.size() - 4] = 0xAB;
    frame.frame_data[frame.frame_data.size() - 3] = 0xCD;
    frame.frame_data[frame.frame_data.size() - 2] = 0xEF;
    frame.frame_data[frame.frame_data.size() - 1] = 0x01;
    
    frame.frame_valid = true;
    return frame;
}();

// Sample corrupted ETI frame implementation
const eti::EtiFrame SAMPLE_CORRUPTED_FRAME = []() {
    eti::EtiFrame frame = SAMPLE_COMPLIANT_FRAME;
    
    // Corrupt the sync pattern
    frame.frame_data[0] = 0x00;
    frame.frame_data[1] = 0x00;
    
    // Corrupt some FIC data
    frame.frame_data[15] = 0xFF;
    frame.frame_data[16] = 0xFF;
    
    frame.frame_valid = false;
    return frame;
}();

// Multi-service ensemble frame
const eti::EtiFrame MULTI_SERVICE_ENSEMBLE_FRAME = []() {
    eti::EtiFrame frame = SAMPLE_COMPLIANT_FRAME;
    
    // Set NST to 3 (3 services)
    frame.frame_data[5] = 0x03;
    
    // Add service information in FIC
    frame.frame_data[16] = 0x01; // Service 1 ID
    frame.frame_data[17] = 0x23;
    frame.frame_data[18] = 0x02; // Service 2 ID  
    frame.frame_data[19] = 0x34;
    frame.frame_data[20] = 0x03; // Service 3 ID
    frame.frame_data[21] = 0x45;
    
    return frame;
}();

// Empty/silent frame
const eti::EtiFrame EMPTY_FRAME = []() {
    eti::EtiFrame frame;
    
    // Valid sync but minimal content
    frame.frame_data[0] = 0xFF;
    frame.frame_data[1] = 0xFF;
    frame.frame_data[2] = 0xFF;
    frame.frame_data[3] = 0xFF;
    
    // Minimal LIDATA
    frame.frame_data[5] = 0x00; // NST=0 (no services)
    
    frame.frame_valid = true;
    return frame;
}();

namespace FicTestData {
    const std::array<uint8_t, 32> VALID_FIC_BLOCK = {{
        0x00, 0x00, 0x12, 0x34, // Ensemble ID
        0x00, 0x56, 0x00, 0x00, // CIF count and flags
        0x00, 0x00, 0x00, 0x00, // Padding
        0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00,
        0xAB, 0xCD, 0xEF, 0x01  // CRC
    }};
    
    const std::array<uint8_t, 32> CORRUPTED_FIC_BLOCK = {{
        0xFF, 0xFF, 0xFF, 0xFF, // Corrupted header
        0xFF, 0xFF, 0xFF, 0xFF,
        0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00  // Invalid CRC
    }};
    
    const std::array<uint8_t, 32> SERVICE_LIST_FIC_BLOCK = {{
        0x00, 0x01, 0x12, 0x34, // Service list header
        0x01, 0x23, 0x02, 0x34, // Service 1 and 2 IDs
        0x03, 0x45, 0x00, 0x00, // Service 3 ID
        0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00,
        0xAB, 0xCD, 0xEF, 0x02  // CRC
    }};
}

namespace MscTestData {
    const std::array<uint8_t, 2048> DAB_AUDIO_SERVICE = []() {
        std::array<uint8_t, 2048> data{};
        // Fill with DAB audio pattern
        for (size_t i = 0; i < data.size(); i++) {
            data[i] = static_cast<uint8_t>((i * 3) & 0xFF);
        }
        return data;
    }();
    
    const std::array<uint8_t, 2048> DABPLUS_AUDIO_SERVICE = []() {
        std::array<uint8_t, 2048> data{};
        // Fill with DAB+ audio pattern
        for (size_t i = 0; i < data.size(); i++) {
            data[i] = static_cast<uint8_t>((i * 5 + 7) & 0xFF);
        }
        return data;
    }();
    
    const std::array<uint8_t, 2048> DATA_SERVICE = []() {
        std::array<uint8_t, 2048> data{};
        // Fill with data service pattern
        for (size_t i = 0; i < data.size(); i++) {
            data[i] = static_cast<uint8_t>((i ^ 0xAA) & 0xFF);
        }
        return data;
    }();
}

namespace TestUtils {
    eti::EtiFrame generateRandomFrame(uint32_t seed) {
        eti::EtiFrame frame = SAMPLE_COMPLIANT_FRAME;
        
        // Use seed to modify frame content predictably
        for (size_t i = 44; i < frame.frame_data.size() - 4; i++) {
            frame.frame_data[i] = static_cast<uint8_t>((seed + i) & 0xFF);
        }
        
        return frame;
    }
    
    eti::EtiFrame createFrameWithService(uint32_t serviceId) {
        eti::EtiFrame frame = SAMPLE_COMPLIANT_FRAME;
        
        // Set service ID in FIC section
        frame.frame_data[16] = static_cast<uint8_t>((serviceId >> 8) & 0xFF);
        frame.frame_data[17] = static_cast<uint8_t>(serviceId & 0xFF);
        
        return frame;
    }
    
    eti::EtiFrame corruptFrame(const eti::EtiFrame& frame, 
                              bool corruptSync,
                              bool corruptFic, 
                              bool corruptMsc) {
        eti::EtiFrame corrupted = frame;
        
        if (corruptSync) {
            corrupted.frame_data[0] = 0x00;
            corrupted.frame_data[1] = 0x00;
        }
        
        if (corruptFic) {
            corrupted.frame_data[12] = 0xFF;
            corrupted.frame_data[13] = 0xFF;
        }
        
        if (corruptMsc) {
            corrupted.frame_data[44] = 0xFF;
            corrupted.frame_data[45] = 0xFF;
        }
        
        corrupted.frame_valid = false;
        return corrupted;
    }
}

} // namespace EtiTestData