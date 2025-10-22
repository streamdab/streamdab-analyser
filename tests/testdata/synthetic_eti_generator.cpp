/**
 * @file synthetic_eti_generator.cpp
 * @brief Synthetic ETI Frame Generator for Testing
 *
 * Generates realistic synthetic ETI frames for integration testing.
 * Provides command-line tool to create test ETI files with known content.
 *
 * Usage:
 *   synthetic_eti_generator [options] output_file.eti
 *
 * Options:
 *   --frames N        Generate N frames (default: 100)
 *   --with-audio      Include audio data in frames
 *   --with-mot        Include MOT data service
 *   --service-label   Set service label
 *
 * Phase 3A Wave 3.1: Integration & Testing Infrastructure
 *
 * @author StreamDAB Development Team
 * @date October 22, 2025
 */

#include <iostream>
#include <fstream>
#include <vector>
#include <cstdint>
#include <cstring>
#include <string>
#include <array>

/**
 * @brief Synthetic ETI Generator Class
 */
class SyntheticETIGenerator {
public:
    /**
     * @brief Generate minimal ETI frame (6144 bytes)
     */
    static std::array<uint8_t, 6144> generateMinimalFrame() {
        std::array<uint8_t, 6144> frame{};
        
        // Sync pattern: ERR=0, FSYNC=0x073, frame_phrase=0xAB6
        frame[0] = 0xFF;
        frame[1] = 0x1F;
        frame[2] = 0x49;
        frame[3] = 0x1F;
        
        // LIDATA (offset 4-7)
        frame[4] = 0x00;  // FCT (Frame Count) = 0
        frame[5] = 0x21;  // FICF=1 (FIC present), NST=1 (1 stream)
        frame[6] = 0x40;  // MID=01 (Mode I), FL=0
        frame[7] = 0x00;  // Padding
        
        // FIC data (offset 8, 96 bytes for Mode I)
        // Fill with FIG Type 0/0 (minimal ensemble info)
        int fic_offset = 8;
        frame[fic_offset] = 0x00;      // FIG Type 0
        frame[fic_offset + 1] = 0x04;  // Length
        frame[fic_offset + 2] = 0x00;  // C/N, OE, PD
        
        // Ensemble ID = 0xE001
        frame[fic_offset + 3] = 0xE0;
        frame[fic_offset + 4] = 0x01;
        
        // Fill rest of FIC with 0xFF (end marker)
        for (int i = fic_offset + 5; i < fic_offset + 96; i++) {
            frame[i] = 0xFF;
        }
        
        // MSC data (rest of frame) - zeros
        
        return frame;
    }
    
    /**
     * @brief Generate ETI frame with audio data
     */
    static std::array<uint8_t, 6144> generateFrameWithAudio(uint8_t frame_count) {
        auto frame = generateMinimalFrame();
        
        // Update frame count
        frame[4] = frame_count;
        
        // Add FIG 0/1 (subchannel organization) to FIC
        int fic_offset = 8 + 6;  // After FIG 0/0
        frame[fic_offset] = 0x01;      // FIG Type 0/1
        frame[fic_offset + 1] = 0x08;  // Length
        frame[fic_offset + 2] = 0x00;  // C/N, OE, PD
        frame[fic_offset + 3] = 0x00;  // SubChId = 0
        frame[fic_offset + 4] = 0x01;  // Audio flag
        
        // MSC: Add synthetic audio superframe structure
        int msc_offset = 8 + 96;
        
        // DAB+ Audio superframe header
        frame[msc_offset] = 0x12;      // FireCode CRC high
        frame[msc_offset + 1] = 0x34;  // FireCode CRC low
        frame[msc_offset + 2] = 0x04;  // num_aus = 4
        frame[msc_offset + 3] = 0x03;  // SBR=1, PS=1 (HE-AAC v2)
        
        // Fill with synthetic audio access units
        for (int i = msc_offset + 4; i < msc_offset + 500; i++) {
            frame[i] = 0xAA ^ (i & 0xFF);  // Pattern
        }
        
        return frame;
    }
    
    /**
     * @brief Generate ETI frame with MOT data service
     */
    static std::array<uint8_t, 6144> generateFrameWithMOT(uint8_t frame_count) {
        auto frame = generateMinimalFrame();
        
        // Update frame count
        frame[4] = frame_count;
        
        // Add FIG 0/2 (service component - data service) to FIC
        int fic_offset = 8 + 6;
        frame[fic_offset] = 0x02;      // FIG Type 0/2
        frame[fic_offset + 1] = 0x0A;  // Length
        frame[fic_offset + 2] = 0x01;  // PD=1 (data service)
        
        // Service ID (32-bit for data)
        frame[fic_offset + 3] = 0xE1;
        frame[fic_offset + 4] = 0xC0;
        frame[fic_offset + 5] = 0x03;
        frame[fic_offset + 6] = 0x79;
        
        // Component data
        frame[fic_offset + 7] = 0x00;   // SCIdS
        frame[fic_offset + 8] = 0x01;   // SubChId = 1
        frame[fic_offset + 9] = 0x3F;   // TMId (MSC data)
        frame[fic_offset + 10] = 0x00;  // DSCTy (MOT)
        
        // MSC: Add MOT data group
        int msc_offset = 8 + 96;
        
        // Data group header
        frame[msc_offset] = 0x50;      // Extension=0, CRC=1, Segment=1
        frame[msc_offset + 1] = 0x03;  // Type = MOT
        frame[msc_offset + 2] = 0x00;  // Continuity/Repetition
        
        // Segment header
        uint16_t segment_size = 64;
        frame[msc_offset + 3] = (segment_size >> 8) & 0x1F;
        frame[msc_offset + 4] = segment_size & 0xFF;
        
        // Segment number
        frame[msc_offset + 5] = 0x00;
        frame[msc_offset + 6] = 0x00;
        
        // Transport ID (10 bits) = 42
        frame[msc_offset + 7] = 0x0A;
        frame[msc_offset + 8] = 0x80;
        
        // Segment data
        for (int i = 0; i < segment_size; i++) {
            frame[msc_offset + 9 + i] = 0xBB ^ (i & 0xFF);
        }
        
        return frame;
    }
    
    /**
     * @brief Generate complete test ETI file
     */
    static bool generateTestFile(const std::string& filename,
                                  int num_frames,
                                  bool with_audio,
                                  bool with_mot) {
        std::ofstream file(filename, std::ios::binary);
        if (!file.is_open()) {
            std::cerr << "Error: Cannot create file " << filename << std::endl;
            return false;
        }
        
        std::cout << "Generating " << num_frames << " frames..." << std::endl;
        
        for (int i = 0; i < num_frames; i++) {
            std::array<uint8_t, 6144> frame;
            
            uint8_t frame_count = i % 250;  // FCT wraps at 250
            
            if (with_audio) {
                frame = generateFrameWithAudio(frame_count);
            } else if (with_mot) {
                frame = generateFrameWithMOT(frame_count);
            } else {
                frame = generateMinimalFrame();
                frame[4] = frame_count;
            }
            
            file.write(reinterpret_cast<const char*>(frame.data()), 6144);
            
            if ((i + 1) % 100 == 0) {
                std::cout << "  Generated " << (i + 1) << " frames..." << std::endl;
            }
        }
        
        file.close();
        
        std::cout << "Successfully created: " << filename << std::endl;
        std::cout << "File size: " << (num_frames * 6144) << " bytes" << std::endl;
        
        return true;
    }
};

/**
 * @brief Main function - command line tool
 */
int main(int argc, char* argv[]) {
    std::cout << "=== Synthetic ETI Generator ===" << std::endl;
    std::cout << "Phase 3A Wave 3.1: Test Data Generation" << std::endl;
    std::cout << std::endl;
    
    // Parse command line arguments
    std::string output_file = "test.eti";
    int num_frames = 100;
    bool with_audio = false;
    bool with_mot = false;
    
    for (int i = 1; i < argc; i++) {
        std::string arg = argv[i];
        
        if (arg == "--frames" && i + 1 < argc) {
            num_frames = std::stoi(argv[++i]);
        } else if (arg == "--with-audio") {
            with_audio = true;
        } else if (arg == "--with-mot") {
            with_mot = true;
        } else if (arg == "--help" || arg == "-h") {
            std::cout << "Usage: " << argv[0] << " [options] [output_file]" << std::endl;
            std::cout << std::endl;
            std::cout << "Options:" << std::endl;
            std::cout << "  --frames N       Generate N frames (default: 100)" << std::endl;
            std::cout << "  --with-audio     Include audio data" << std::endl;
            std::cout << "  --with-mot       Include MOT data service" << std::endl;
            std::cout << "  --help, -h       Show this help" << std::endl;
            std::cout << std::endl;
            std::cout << "Examples:" << std::endl;
            std::cout << "  " << argv[0] << " --frames 1000 --with-audio test_audio.eti" << std::endl;
            std::cout << "  " << argv[0] << " --frames 500 --with-mot test_mot.eti" << std::endl;
            return 0;
        } else if (arg[0] != '-') {
            output_file = arg;
        }
    }
    
    // Generate test file
    std::cout << "Configuration:" << std::endl;
    std::cout << "  Output file: " << output_file << std::endl;
    std::cout << "  Frames: " << num_frames << std::endl;
    std::cout << "  Audio data: " << (with_audio ? "Yes" : "No") << std::endl;
    std::cout << "  MOT data: " << (with_mot ? "Yes" : "No") << std::endl;
    std::cout << std::endl;
    
    bool success = SyntheticETIGenerator::generateTestFile(
        output_file, num_frames, with_audio, with_mot
    );
    
    return success ? 0 : 1;
}
