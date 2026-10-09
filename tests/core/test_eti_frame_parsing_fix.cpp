/**
 * @file test_eti_frame_parsing_fix.cpp
 * @brief TDD Tests for ETI Frame Parsing Issue Fix
 * 
 * This test identifies and fixes the critical issue where ALL ETI frames
 * fail to parse (0/5001 frames processed) in the Bangkok broadcast file.
 * 
 * RED-GREEN-REFACTOR approach:
 * 1. RED: Tests fail because current parser cannot parse any frames
 * 2. GREEN: Fix parser implementation to make tests pass
 * 3. REFACTOR: Optimize while maintaining passing tests
 * 
 * @author ETI Standards Compliance Agent
 * @date 2025
 */

#include <gtest/gtest.h>
#include <QCoreApplication>
#include <QByteArray>
#include <QDebug>
#include <array>
#include <chrono>

#include "core/modern_eti_frame_parser.hpp"
#include "core/eti_types.h"

using namespace eti;
using namespace eti::modern;

class ETIFrameParsingFixTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Initialize Qt application if needed
        if (!QCoreApplication::instance()) {
            static int argc = 1;
            static char* argv[] = {"test"};
            app = std::make_unique<QCoreApplication>(argc, argv);
        }
        
        // Create parser with default configuration
        ProcessingConfig config;
        config.target_fps = 1000.0;
        config.optimization_level = ProcessingConfig::OptimizationLevel::Balanced;
        config.enable_threading = false;  // Disable for consistent testing
        config.enable_caching = false;
        config.enable_memory_pooling = false;
        
        parser = std::make_unique<ModernETIFrameParser>();
        ASSERT_TRUE(parser->initialize(config));
    }
    
    void TearDown() override {
        parser.reset();
    }
    
    /**
     * @brief Create a valid ETI frame with proper structure
     * 
     * This creates a minimal valid ETI frame that should parse successfully:
     * - Valid sync pattern: {0x49, 0x93, 0x1E, 0x03}
     * - Valid LIDATA field with reasonable values
     * - FIC field with basic ensemble information
     * - MSC field with some data
     * - Valid CRC (for now, just placeholder)
     */
    std::array<uint8_t, ETI_FRAME_SIZE> createValidETIFrame() {
        std::array<uint8_t, ETI_FRAME_SIZE> frame_data{};
        frame_data.fill(0);
        
        // 1. SYNC field (bytes 0-3): ETI sync pattern
        frame_data[0] = 0x49;
        frame_data[1] = 0x93;
        frame_data[2] = 0x1E;
        frame_data[3] = 0x03;
        
        // 2. LIDATA field (bytes 4-11): Basic valid values
        frame_data[4] = 0x00;  // FC (Frame Count) = 0
        frame_data[5] = 0x83;  // FICF=1, NST=3 (FIC present, 3 sub-channels)
        frame_data[6] = 0x20;  // FP=1, MID=1, FL upper bits
        frame_data[7] = 0x00;  // FL lower bits
        
        // TIST (Time Stamp) - 32-bit value (bytes 8-11)
        frame_data[8] = 0x12;
        frame_data[9] = 0x34;
        frame_data[10] = 0x56;
        frame_data[11] = 0x78;
        
        // 3. FIC field (bytes 12-43): Simple ensemble info
        // FIG 0/0: Ensemble information
        frame_data[12] = 0x08;  // FIG header: type=0, length=8
        frame_data[13] = 0x00;  // Extension=0
        frame_data[14] = 0x10;  // Ensemble ID high byte
        frame_data[15] = 0x01;  // Ensemble ID low byte
        frame_data[16] = 0x90;  // Country ID=9, ECC=0
        frame_data[17] = 0x00;  // CIF count
        frame_data[18] = 0x00;  // Occurrence change
        frame_data[19] = 0x00;  // Reserved
        
        // End FIG marker
        frame_data[20] = 0xFF;
        
        // Rest of FIC is zeros (padding)
        
        // 4. MSC field (bytes 44-6139): Some basic data
        // Put some non-zero data to make it realistic
        for (size_t i = 44; i < 44 + 100; ++i) {
            frame_data[i] = static_cast<uint8_t>(i & 0xFF);
        }
        
        // 5. CRC field (bytes 6140-6143): Placeholder CRC
        frame_data[6140] = 0xAB;
        frame_data[6141] = 0xCD;
        frame_data[6142] = 0xEF;
        frame_data[6143] = 0x12;
        
        return frame_data;
    }
    
    /**
     * @brief Create an invalid ETI frame with wrong sync pattern
     */
    std::array<uint8_t, ETI_FRAME_SIZE> createInvalidETIFrame() {
        auto frame = createValidETIFrame();
        
        // Corrupt sync pattern
        frame[0] = 0x00;  // Wrong sync pattern
        frame[1] = 0x00;
        frame[2] = 0x00;
        frame[3] = 0x00;
        
        return frame;
    }
    
    /**
     * @brief Create a Bangkok ETI-like frame based on known characteristics
     * 
     * The Bangkok ETI file has specific characteristics:
     * - 5001 frames total
     * - Contains Thai broadcast services
     * - Should parse successfully with proper implementation
     */
    std::array<uint8_t, ETI_FRAME_SIZE> createBangkokETIFrame() {
        std::array<uint8_t, ETI_FRAME_SIZE> frame_data{};
        frame_data.fill(0);
        
        // Valid ETI sync pattern
        frame_data[0] = 0x49;
        frame_data[1] = 0x93;
        frame_data[2] = 0x1E;
        frame_data[3] = 0x03;
        
        // LIDATA with typical Bangkok broadcast parameters
        frame_data[4] = 0x42;  // FC = 66 (frame in sequence)
        frame_data[5] = 0x83;  // FICF=1, NST=3 (3 services typical for Bangkok)
        frame_data[6] = 0x21;  // FP=1, MID=1 (DAB Mode I)
        frame_data[7] = 0x00;  // FL
        
        // TIST with realistic timestamp
        uint32_t tist = 0x12345678;
        frame_data[8] = (tist >> 24) & 0xFF;
        frame_data[9] = (tist >> 16) & 0xFF;
        frame_data[10] = (tist >> 8) & 0xFF;
        frame_data[11] = tist & 0xFF;
        
        // FIC with Bangkok ensemble info
        // FIG 0/0: Thailand ensemble
        frame_data[12] = 0x08;  // FIG header
        frame_data[13] = 0x00;  // Extension 0
        frame_data[14] = 0xE1;  // Thailand ensemble ID
        frame_data[15] = 0x01;
        frame_data[16] = 0x90;  // Country=9 (Thailand)
        frame_data[17] = 0x42;  // CIF count
        frame_data[18] = 0x00;  // Occurrence change
        frame_data[19] = 0x00;
        
        // FIG 0/2: Service organization (3 services)
        frame_data[20] = 0x0C;  // FIG header: length=12
        frame_data[21] = 0x02;  // Extension 2
        // Service 1: Radio Thailand
        frame_data[22] = 0xE1;  // Service ID high
        frame_data[23] = 0x01;  // Service ID low
        frame_data[24] = 0x90;  // Country=9
        frame_data[25] = 0x02;  // Programme service
        // Service 2: NBT World
        frame_data[26] = 0xE1;
        frame_data[27] = 0x02;
        frame_data[28] = 0x90;
        frame_data[29] = 0x02;
        // Service 3: Voice of America Thai
        frame_data[30] = 0xE1;
        frame_data[31] = 0x03;
        frame_data[32] = 0x90;
        frame_data[33] = 0x02;
        
        // End marker
        frame_data[34] = 0xFF;
        
        // MSC with audio data simulation
        for (size_t i = 44; i < 44 + 1000; ++i) {
            frame_data[i] = static_cast<uint8_t>((i * 7 + 123) & 0xFF);  // Pseudo-audio pattern
        }
        
        // CRC (placeholder - in real implementation would be calculated)
        frame_data[6140] = 0x12;
        frame_data[6141] = 0x34;
        frame_data[6142] = 0x56;
        frame_data[6143] = 0x78;
        
        return frame_data;
    }
    
    std::unique_ptr<QCoreApplication> app;
    std::unique_ptr<ModernETIFrameParser> parser;
};

// ============================================================================
// TDD Test 1: Basic ETI Frame Structure Validation (RED Phase)
// ============================================================================

TEST_F(ETIFrameParsingFixTest, ValidateBasicETIFrameStructure) {
    // ARRANGE: Create a basic valid ETI frame
    auto frame_data = createValidETIFrame();
    
    // ACT: Parse the frame
    auto result = parser->parseFrame(frame_data);
    
    // ASSERT: Frame should parse successfully
    EXPECT_TRUE(result.success) << "Basic valid ETI frame should parse successfully";
    EXPECT_GT(result.frame_number, 0) << "Frame number should be assigned";
    EXPECT_LT(result.parse_time.count(), 100000) << "Parse time should be under 100μs";
    
    // Verify frame structure
    EXPECT_TRUE(result.frame.validate_sync_pattern()) << "Sync pattern should be valid";
    
    auto lidata = result.frame.get_lidata_field();
    EXPECT_EQ(lidata.fc, 0) << "Frame count should match";
    EXPECT_EQ(lidata.ficf, 1) << "FIC flag should be set";
    EXPECT_EQ(lidata.nst, 3) << "Number of sub-channels should match";
    EXPECT_EQ(lidata.mid, 1) << "Mode identity should be DAB Mode I";
}

TEST_F(ETIFrameParsingFixTest, RejectInvalidETIFrameStructure) {
    // ARRANGE: Create an invalid ETI frame
    auto frame_data = createInvalidETIFrame();
    
    // ACT: Parse the frame
    auto result = parser->parseFrame(frame_data);
    
    // ASSERT: Frame should be rejected
    EXPECT_FALSE(result.success) << "Invalid ETI frame should be rejected";
    EXPECT_FALSE(result.frame.validate_sync_pattern()) << "Invalid sync pattern should be detected";
}

// ============================================================================
// TDD Test 2: Bangkok ETI Frame Parsing (RED Phase - Critical Test)
// ============================================================================

TEST_F(ETIFrameParsingFixTest, ParseBangkokETIFrameSuccess) {
    // ARRANGE: Create Bangkok-style ETI frame
    auto frame_data = createBangkokETIFrame();
    
    // ACT: Parse the frame
    auto result = parser->parseFrame(frame_data);
    
    // ASSERT: Frame should parse successfully (THIS IS THE CRITICAL TEST)
    EXPECT_TRUE(result.success) << "Bangkok ETI frame should parse successfully - this is failing currently!";
    
    if (result.success) {
        // Validate Bangkok-specific characteristics
        auto lidata = result.frame.get_lidata_field();
        EXPECT_EQ(lidata.ficf, 1) << "Bangkok frames should have FIC data";
        EXPECT_EQ(lidata.nst, 3) << "Bangkok typically has 3 services";
        EXPECT_EQ(lidata.mid, 1) << "Bangkok uses DAB Mode I";
        
        // FIG analysis should find services
        auto fig_result = parser->analyzeFIGData(result.frame);
        EXPECT_GE(fig_result.discovered_services.size(), 1) << "Should discover at least one service";
        
        // Performance check
        EXPECT_LT(result.parse_time.count(), 50000) << "Parse time should be under 50μs for efficiency";
    } else {
        // Log debugging information for the current failure
        qDebug() << "Bangkok frame parsing failed - debugging info:";
        qDebug() << "Frame starts with:" 
                 << QString::number(frame_data[0], 16)
                 << QString::number(frame_data[1], 16)
                 << QString::number(frame_data[2], 16)
                 << QString::number(frame_data[3], 16);
    }
}

// ============================================================================
// TDD Test 3: ETI Frame Header Validation (Specific Failure Point)
// ============================================================================

TEST_F(ETIFrameParsingFixTest, ValidateETIFrameHeaderParsing) {
    // ARRANGE: Create frame with known header values
    auto frame_data = createValidETIFrame();
    
    // ACT: Parse just the header validation
    auto result = parser->parseFrame(frame_data);
    
    // ASSERT: Header validation should work
    if (result.success) {
        auto sync = result.frame.get_sync_field();
        EXPECT_TRUE(sync.is_valid()) << "Sync field validation should pass";
        
        auto lidata = result.frame.get_lidata_field();
        EXPECT_LE(lidata.fc, 249) << "Frame count should be in valid range (0-249)";
        EXPECT_LE(lidata.nst, 63) << "Sub-channel count should be in valid range (0-63)";
        EXPECT_GE(lidata.mid, 1) << "Mode identity should be >= 1";
        EXPECT_LE(lidata.mid, 4) << "Mode identity should be <= 4";
        EXPECT_LE(lidata.fp, 7) << "Frame phase should be <= 7";
    }
}

// ============================================================================
// TDD Test 4: Performance Requirements (GREEN Phase Target)
// ============================================================================

TEST_F(ETIFrameParsingFixTest, MeetPerformanceRequirements) {
    // ARRANGE: Create multiple Bangkok-style frames
    std::vector<std::array<uint8_t, ETI_FRAME_SIZE>> frames;
    for (int i = 0; i < 100; ++i) {
        auto frame = createBangkokETIFrame();
        frame[4] = static_cast<uint8_t>(i % 250);  // Vary frame count
        frames.push_back(frame);
    }
    
    // ACT: Parse all frames and measure performance
    auto start_time = std::chrono::high_resolution_clock::now();
    int successful_parses = 0;
    
    for (const auto& frame_data : frames) {
        auto result = parser->parseFrame(frame_data);
        if (result.success) {
            successful_parses++;
        }
    }
    
    auto end_time = std::chrono::high_resolution_clock::now();
    auto total_time = std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time);
    
    // ASSERT: Performance requirements
    EXPECT_EQ(successful_parses, 100) << "All Bangkok-style frames should parse successfully";
    
    if (successful_parses > 0) {
        double avg_time_us = static_cast<double>(total_time.count()) / successful_parses;
        double fps = 1000000.0 / avg_time_us;  // Convert to FPS
        
        EXPECT_LT(avg_time_us, 25.0) << "Average parse time should be under 25μs";
        EXPECT_GT(fps, 40000.0) << "Should achieve >40k FPS parsing speed";
        
        qDebug() << "Performance results:"
                 << "Successful parses:" << successful_parses << "/100"
                 << "Average time:" << avg_time_us << "μs"
                 << "Effective FPS:" << fps;
    }
}

// ============================================================================
// TDD Test 5: Service Discovery from Bangkok ETI (Integration Test)
// ============================================================================

TEST_F(ETIFrameParsingFixTest, DiscoverBangkokServices) {
    // ARRANGE: Create Bangkok frame with known services
    auto frame_data = createBangkokETIFrame();
    
    // ACT: Parse frame and analyze FIG data
    auto parse_result = parser->parseFrame(frame_data);
    ASSERT_TRUE(parse_result.success) << "Frame parsing must succeed for service discovery";
    
    auto fig_result = parser->analyzeFIGData(parse_result.frame);
    
    // ASSERT: Should discover expected Bangkok services
    EXPECT_GE(fig_result.discovered_services.size(), 1) << "Should discover at least one service";
    
    // Check for typical Bangkok service characteristics
    bool found_thai_service = false;
    for (const auto& service : fig_result.discovered_services) {
        if (service.country_id == 9) {  // Thailand country code
            found_thai_service = true;
            EXPECT_TRUE(service.validate_service_id()) << "Thai service ID should be valid";
            EXPECT_TRUE(service.validate_country_code()) << "Thai country code should be valid";
        }
    }
    
    EXPECT_TRUE(found_thai_service) << "Should find at least one Thai service";
    
    // ETSI compliance check
    EXPECT_GT(fig_result.compliance.etsi_en_300_799_score, 80.0) << "Should meet basic ETSI 799 compliance";
    EXPECT_GT(fig_result.compliance.etsi_en_300_401_score, 80.0) << "Should meet basic ETSI 401 compliance";
}

// ============================================================================
// TDD Test 6: QByteArray Interface Compatibility
// ============================================================================

TEST_F(ETIFrameParsingFixTest, ParseFrameFromQByteArray) {
    // ARRANGE: Create frame as QByteArray (GUI interface compatibility)
    auto frame_data = createValidETIFrame();
    QByteArray qframe(reinterpret_cast<const char*>(frame_data.data()), ETI_FRAME_SIZE);
    
    // ACT: Parse using QByteArray interface
    auto result = parser->parseFrame(qframe);
    
    // ASSERT: Should work the same as array interface
    EXPECT_TRUE(result.success) << "QByteArray interface should work";
    EXPECT_EQ(qframe.size(), ETI_FRAME_SIZE) << "Frame size should be preserved";
}

TEST_F(ETIFrameParsingFixTest, RejectInvalidQByteArraySize) {
    // ARRANGE: Create QByteArray with wrong size
    QByteArray invalid_frame(1000, 0x00);  // Wrong size
    
    // ACT: Try to parse
    auto result = parser->parseFrame(invalid_frame);
    
    // ASSERT: Should be rejected due to size
    EXPECT_FALSE(result.success) << "Invalid frame size should be rejected";
}

// ============================================================================
// Main function for running tests
// ============================================================================

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}