#include "sample_eti_frame.h"
#include <algorithm>
#include <cstring>
#include <cstdlib>
#include <random>

namespace EtiTestData {

// ETI Frame implementation
EtiFrame::EtiFrame() {
    std::memset(data, 0, FRAME_SIZE);
    
    // Set frame synchronization pattern (ETSI EN 300 799)
    data[0] = 0xFF;
    data[1] = 0xFF; 
    data[2] = 0xFF;
    data[3] = 0xFF;
}

bool EtiFrame::isValid() const {
    // Check synchronization pattern
    return (data[0] == 0xFF && data[1] == 0xFF && 
            data[2] == 0xFF && data[3] == 0xFF);
}

// Sample compliant ETI frame for TDD testing
const EtiFrame SAMPLE_COMPLIANT_FRAME = []() {
    EtiFrame frame;
    
    // Frame synchronization (bytes 0-3)
    frame.data[0] = 0xFF;
    frame.data[1] = 0xFF;
    frame.data[2] = 0xFF;
    frame.data[3] = 0xFF;
    
    // Frame Header (bytes 4-11)
    // ERR field (3 bits) + Frame Count (8 bits) + reserved (5 bits)
    frame.data[4] = 0x00; // ERR=0, FC high bits
    frame.data[5] = 0x01; // FC=1 (frame counter)
    
    // NST (Number of streams) = 1
    frame.data[6] = 0x01;
    
    // FP (Frame Phase) = 0, MID (Mode Identity) = 1 (Mode I)
    frame.data[7] = 0x01;
    
    // FL (Frame Length) = 6144 - 1 = 6143 (0x17FF)
    frame.data[8] = 0x17;
    frame.data[9] = 0xFF;
    
    // CRC (2 bytes) - simplified for testing
    frame.data[10] = 0x12;
    frame.data[11] = 0x34;
    
    // Stream data area (bytes 12 to 6139)
    // Add a simple audio service data pattern
    for (size_t i = 12; i < FRAME_SIZE - EOF_CRC_SIZE; i++) {
        frame.data[i] = static_cast<uint8_t>((i - 12) % 256);
    }
    
    // EOF + CRC (last 4 bytes)
    frame.data[FRAME_SIZE - 4] = 0xFF;
    frame.data[FRAME_SIZE - 3] = 0xFF;
    frame.data[FRAME_SIZE - 2] = 0xAB; // CRC placeholder
    frame.data[FRAME_SIZE - 1] = 0xCD; // CRC placeholder
    
    return frame;
}();

// Sample corrupted ETI frame for error testing
const EtiFrame SAMPLE_CORRUPTED_FRAME = []() {
    EtiFrame frame = SAMPLE_COMPLIANT_FRAME;
    
    // Corrupt the synchronization pattern
    frame.data[0] = 0x00;
    frame.data[1] = 0xFF;
    
    // Corrupt some data
    frame.data[100] = 0xFF;
    frame.data[101] = 0xFF;
    
    return frame;
}();

// Multi-service ETI frame
const EtiFrame MULTI_SERVICE_FRAME = []() {
    EtiFrame frame;
    
    // Frame synchronization
    frame.data[0] = 0xFF;
    frame.data[1] = 0xFF;
    frame.data[2] = 0xFF;
    frame.data[3] = 0xFF;
    
    // Frame Header with multiple services
    frame.data[4] = 0x00; // ERR=0
    frame.data[5] = 0x02; // FC=2
    frame.data[6] = 0x03; // NST=3 (3 services)
    frame.data[7] = 0x01; // FP=0, MID=1
    frame.data[8] = 0x17; // FL high byte
    frame.data[9] = 0xFF; // FL low byte
    frame.data[10] = 0x56; // CRC
    frame.data[11] = 0x78; // CRC
    
    // Service data for 3 audio services
    size_t offset = 12;
    for (int service = 0; service < 3; service++) {
        for (size_t i = 0; i < 2000 && offset < FRAME_SIZE - 4; i++, offset++) {
            frame.data[offset] = static_cast<uint8_t>((service * 100 + i) % 256);
        }
    }
    
    // Fill remaining with padding
    while (offset < FRAME_SIZE - 4) {
        frame.data[offset++] = 0x55; // Padding pattern
    }
    
    // EOF + CRC
    frame.data[FRAME_SIZE - 4] = 0xFF;
    frame.data[FRAME_SIZE - 3] = 0xFF;
    frame.data[FRAME_SIZE - 2] = 0xEF;
    frame.data[FRAME_SIZE - 1] = 0xBE;
    
    return frame;
}();

// Empty ETI frame template
const EtiFrame EMPTY_FRAME = []() {
    EtiFrame frame;
    
    // Only synchronization and minimal header
    frame.data[0] = 0xFF;
    frame.data[1] = 0xFF;
    frame.data[2] = 0xFF;
    frame.data[3] = 0xFF;
    
    // Minimal header
    frame.data[4] = 0x00; // ERR=0
    frame.data[5] = 0x00; // FC=0
    frame.data[6] = 0x00; // NST=0 (no services)
    frame.data[7] = 0x01; // FP=0, MID=1
    frame.data[8] = 0x17; // FL
    frame.data[9] = 0xFF; // FL
    frame.data[10] = 0x00; // CRC
    frame.data[11] = 0x00; // CRC
    
    // Fill with null pattern
    std::memset(&frame.data[12], 0x00, FRAME_SIZE - 12 - 4);
    
    // EOF + CRC
    frame.data[FRAME_SIZE - 4] = 0xFF;
    frame.data[FRAME_SIZE - 3] = 0xFF;
    frame.data[FRAME_SIZE - 2] = 0x00;
    frame.data[FRAME_SIZE - 1] = 0x00;
    
    return frame;
}();

// Performance test stream generator
std::vector<EtiFrame> PerformanceTestStream::generate() {
    std::vector<EtiFrame> frames;
    frames.reserve(FRAME_COUNT);
    
    for (size_t i = 0; i < FRAME_COUNT; i++) {
        EtiFrame frame = SAMPLE_COMPLIANT_FRAME;
        
        // Update frame counter
        frame.data[5] = static_cast<uint8_t>(i % 256);
        frame.data[4] = static_cast<uint8_t>((i >> 8) % 8); // ERR + high FC bits
        
        // Add some variation to the data
        for (size_t j = 12; j < EtiFrame::FRAME_SIZE - 4; j += 100) {
            frame.data[j] = static_cast<uint8_t>((i + j) % 256);
        }
        
        frames.push_back(frame);
    }
    
    return frames;
}

std::vector<uint8_t> PerformanceTestStream::generateRawData() {
    auto frames = generate();
    std::vector<uint8_t> rawData;
    rawData.reserve(FRAME_COUNT * EtiFrame::FRAME_SIZE);
    
    for (const auto& frame : frames) {
        rawData.insert(rawData.end(), frame.data, frame.data + EtiFrame::FRAME_SIZE);
    }
    
    return rawData;
}

// FIC test data
namespace FicTestData {
    // FIG Type 0/0: Basic ensemble information
    const std::vector<uint8_t> FIG_0_0_ENSEMBLE_INFO = {
        0x00, 0x08, // FIG header: Type 0/0, length 8
        0x12, 0x34, // Ensemble ID (EId) = 0x1234
        0x05,       // Change flag=0, Alarm=0, CIF Count high=0, CIF Count low=5
        0x00,       // CIF Count continuation
        0x01,       // Number of services
        0x00        // Reserved
    };
    
    // FIG Type 0/1: Basic sub-channel organization  
    const std::vector<uint8_t> FIG_0_1_SUBCHANNEL_ORG = {
        0x01, 0x0A, // FIG header: Type 0/1, length 10
        0x01,       // Sub-channel ID = 1
        0x00, 0x10, // Start address = 16
        0x2C,       // Table switch=0, Table index=1, Option=1, Protection level=1, Sub-ch size high=0
        0x40,       // Sub-channel size low = 64 (CUs)
        0x00, 0x00, 0x00 // Padding
    };
    
    // FIG Type 0/2: Basic service organization
    const std::vector<uint8_t> FIG_0_2_SERVICE_ORG = {
        0x02, 0x08, // FIG header: Type 0/2, length 8
        0x00, 0x01, // Country code=0, Service reference=1
        0x12, 0x34, // Service ID = 0x1234
        0x01,       // Sub-channel ID = 1
        0x00        // Padding
    };
    
    // FIG Type 1/0: Service labels
    const std::vector<uint8_t> FIG_1_0_SERVICE_LABELS = {
        0x10, 0x16, // FIG header: Type 1/0, length 22
        0x12, 0x34, // Service ID = 0x1234
        'T', 'E', 'S', 'T', ' ', 'S', 'E', 'R', 'V', 'I', 'C', 'E', ' ', ' ', ' ', ' ', // Label (16 chars)
        0x00, 0xF0  // Character flag field
    };
    
    // Complete FIC block with multiple FIGs
    const std::vector<uint8_t> COMPLETE_FIC_BLOCK = []() {
        std::vector<uint8_t> fic;
        
        // Add all FIG types
        fic.insert(fic.end(), FIG_0_0_ENSEMBLE_INFO.begin(), FIG_0_0_ENSEMBLE_INFO.end());
        fic.insert(fic.end(), FIG_0_1_SUBCHANNEL_ORG.begin(), FIG_0_1_SUBCHANNEL_ORG.end());
        fic.insert(fic.end(), FIG_0_2_SERVICE_ORG.begin(), FIG_0_2_SERVICE_ORG.end());
        fic.insert(fic.end(), FIG_1_0_SERVICE_LABELS.begin(), FIG_1_0_SERVICE_LABELS.end());
        
        // Pad to FIC block size (32 bytes for Mode I)
        while (fic.size() < 32) {
            fic.push_back(0xFF); // Null padding
        }
        
        return fic;
    }();
}

// Audio test data
namespace AudioTestData {
    // Sample DAB audio frame (MP2) - simplified for testing
    const std::vector<uint8_t> SAMPLE_DAB_AUDIO_FRAME = {
        0xFF, 0xFB, 0x90, 0x00, // MP2 sync word + header
        0x00, 0x00, 0x00, 0x00, // Audio data (simplified)
        0x01, 0x23, 0x45, 0x67, 
        0x89, 0xAB, 0xCD, 0xEF,
        // ... additional audio data would follow
    };
    
    // Sample DAB+ audio frame (AAC) - simplified for testing
    const std::vector<uint8_t> SAMPLE_DABPLUS_AUDIO_FRAME = {
        0x56, 0xE0, // AAC ADTS header (simplified)
        0x00, 0x1F, 0xFC, // AAC frame header
        0x21, 0x10, 0x04, 0x60, // AAC audio data (simplified)
        0x8C, 0x1C, 0x30, 0x00,
        // ... additional AAC data would follow
    };
    
    // Corrupted audio frame for error testing
    const std::vector<uint8_t> CORRUPTED_AUDIO_FRAME = {
        0x00, 0x00, 0x00, 0x00, // Invalid sync
        0xFF, 0xFF, 0xFF, 0xFF, // Corrupted data
        0x55, 0xAA, 0x55, 0xAA, // Pattern corruption
    };
}

// Test data generator implementation
EtiFrame TestDataGenerator::generateValidFrame(uint32_t frameNumber, uint8_t serviceCount) {
    EtiFrame frame;
    
    // Frame sync
    frame.data[0] = 0xFF;
    frame.data[1] = 0xFF;
    frame.data[2] = 0xFF;
    frame.data[3] = 0xFF;
    
    // Header with frame number
    frame.data[4] = static_cast<uint8_t>((frameNumber >> 8) & 0x07);
    frame.data[5] = static_cast<uint8_t>(frameNumber & 0xFF);
    frame.data[6] = serviceCount; // NST
    frame.data[7] = 0x01; // Mode I
    frame.data[8] = 0x17; // Frame length
    frame.data[9] = 0xFF;
    frame.data[10] = 0x12; // CRC
    frame.data[11] = 0x34;
    
    // Generate service data
    size_t offset = 12;
    for (uint8_t service = 0; service < serviceCount && offset < EtiFrame::FRAME_SIZE - 4; service++) {
        for (size_t i = 0; i < 2000 && offset < EtiFrame::FRAME_SIZE - 4; i++, offset++) {
            frame.data[offset] = static_cast<uint8_t>((frameNumber + service * 256 + i) % 256);
        }
    }
    
    // Fill remaining with padding
    while (offset < EtiFrame::FRAME_SIZE - 4) {
        frame.data[offset++] = 0x55;
    }
    
    // EOF + CRC
    frame.data[EtiFrame::FRAME_SIZE - 4] = 0xFF;
    frame.data[EtiFrame::FRAME_SIZE - 3] = 0xFF;
    frame.data[EtiFrame::FRAME_SIZE - 2] = 0xAB;
    frame.data[EtiFrame::FRAME_SIZE - 1] = 0xCD;
    
    return frame;
}

EtiFrame TestDataGenerator::generateErrorFrame(ErrorType errorType) {
    EtiFrame frame = generateValidFrame(1, 1);
    
    switch (errorType) {
        case ErrorType::INVALID_SYNC:
            frame.data[0] = 0x00;
            frame.data[1] = 0x00;
            break;
            
        case ErrorType::CORRUPTED_HEADER:
            frame.data[6] = 0xFF; // Invalid NST
            frame.data[7] = 0xFF; // Invalid mode
            break;
            
        case ErrorType::INVALID_CRC:
            frame.data[EtiFrame::FRAME_SIZE - 2] = 0x00;
            frame.data[EtiFrame::FRAME_SIZE - 1] = 0x00;
            break;
            
        case ErrorType::TRUNCATED_FRAME:
            // Can't actually truncate in fixed-size array, 
            // but we can mark it as truncated
            std::memset(&frame.data[1000], 0x00, EtiFrame::FRAME_SIZE - 1000);
            break;
            
        case ErrorType::OVERSIZED_FRAME:
            // Simulate oversized by corrupting length field
            frame.data[8] = 0xFF;
            frame.data[9] = 0xFF;
            break;
    }
    
    return frame;
}

std::vector<EtiFrame> TestDataGenerator::generateStream(size_t frameCount, bool withErrors) {
    std::vector<EtiFrame> frames;
    frames.reserve(frameCount);
    
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> errorDist(0, 4);
    std::uniform_int_distribution<> shouldErrorDist(0, 19); // 5% error rate
    
    for (size_t i = 0; i < frameCount; i++) {
        if (withErrors && shouldErrorDist(gen) == 0) {
            // Generate error frame
            auto errorType = static_cast<ErrorType>(errorDist(gen));
            frames.push_back(generateErrorFrame(errorType));
        } else {
            // Generate valid frame
            frames.push_back(generateValidFrame(static_cast<uint32_t>(i), 1));
        }
    }
    
    return frames;
}

} // namespace EtiTestData