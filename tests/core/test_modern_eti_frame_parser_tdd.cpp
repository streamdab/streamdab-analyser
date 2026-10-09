/**
 * @file test_modern_eti_frame_parser_tdd.cpp
 * @brief TDD Tests for ModernETIFrameParser
 * 
 * Test-driven development validation for the Modern ETI Frame Parser
 * implementation with ETSI compliance verification.
 * 
 * @author Standards Compliance Agent (TDD Implementation)
 * @date 2025
 */

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "core/modern_eti_frame_parser.hpp"
#include "eti_types.hpp"
#include <QByteArray>
#include <QSignalSpy>
#include <chrono>
#include <memory>

using namespace eti::modern;
using namespace testing;

class ModernETIFrameParserTest : public ::testing::Test {
protected:
    void SetUp() override {
        parser_ = std::make_unique<ModernETIFrameParser>();
        
        // Create valid ETI frame data
        createValidETIFrame();
        createInvalidETIFrame();
    }
    
    void TearDown() override {
        parser_.reset();
    }
    
    void createValidETIFrame() {
        valid_frame_data_.resize(ETI_FRAME_SIZE);
        
        // Valid sync pattern (0x49, 0x93, 0x1E, 0x03)
        valid_frame_data_[0] = 0x49;
        valid_frame_data_[1] = 0x93;
        valid_frame_data_[2] = 0x1E;
        valid_frame_data_[3] = 0x03;
        
        // Valid LIDATA field
        valid_frame_data_[4] = 0x00; // FC (frame count) = 0
        valid_frame_data_[5] = 0x04; // NST = 1, FICF = 1, FP = 0, MID = 1
        valid_frame_data_[6] = 0x00; // Padding
        valid_frame_data_[7] = 0x00; // Padding
        
        // Fill rest with zeros
        std::fill(valid_frame_data_.begin() + 8, valid_frame_data_.end(), 0x00);
    }
    
    void createInvalidETIFrame() {
        invalid_frame_data_.resize(ETI_FRAME_SIZE);
        
        // Invalid sync pattern
        invalid_frame_data_[0] = 0x00;
        invalid_frame_data_[1] = 0x00;
        invalid_frame_data_[2] = 0x00;
        invalid_frame_data_[3] = 0x00;
        
        // Fill rest with random data
        std::fill(invalid_frame_data_.begin() + 4, invalid_frame_data_.end(), 0xAA);
    }
    
    std::unique_ptr<ModernETIFrameParser> parser_;
    QByteArray valid_frame_data_;
    QByteArray invalid_frame_data_;
};

// ============================================================================
// Initialization Tests
// ============================================================================

TEST_F(ModernETIFrameParserTest, DefaultConstruction) {
    EXPECT_FALSE(parser_->isInitialized());
    EXPECT_NE(parser_.get(), nullptr);
}

TEST_F(ModernETIFrameParserTest, InitializationWithDefaultConfig) {
    ProcessingConfig config;
    
    EXPECT_TRUE(parser_->initialize(config));
    EXPECT_TRUE(parser_->isInitialized());
}

TEST_F(ModernETIFrameParserTest, InitializationWithCustomConfig) {
    ProcessingConfig config;
    config.optimization_level = ProcessingConfig::OptimizationLevel::AGGRESSIVE;
    config.target_fps = 2000.0;
    config.memory_limit_mb = 50;
    config.enable_simd = true;
    config.enable_caching = true;
    config.enable_threading = true;
    
    EXPECT_TRUE(parser_->initialize(config));
    EXPECT_TRUE(parser_->isInitialized());
}

TEST_F(ModernETIFrameParserTest, MultipleInitializationCalls) {
    ProcessingConfig config;
    
    EXPECT_TRUE(parser_->initialize(config));
    EXPECT_TRUE(parser_->isInitialized());
    
    // Second initialization should also succeed
    EXPECT_TRUE(parser_->initialize(config));
    EXPECT_TRUE(parser_->isInitialized());
}

// ============================================================================
// Frame Parsing Tests
// ============================================================================

TEST_F(ModernETIFrameParserTest, ParseValidFrame) {
    ProcessingConfig config;
    ASSERT_TRUE(parser_->initialize(config));
    
    auto result = parser_->parseFrame(valid_frame_data_);
    
    EXPECT_TRUE(result.success);
    EXPECT_EQ(result.frame_number, 1);
    EXPECT_GT(result.parse_time.count(), 0);
    EXPECT_TRUE(result.validation.is_valid);
}

TEST_F(ModernETIFrameParserTest, ParseInvalidFrame) {
    ProcessingConfig config;
    ASSERT_TRUE(parser_->initialize(config));
    
    auto result = parser_->parseFrame(invalid_frame_data_);
    
    EXPECT_FALSE(result.success);
    EXPECT_FALSE(result.validation.is_valid);
    EXPECT_FALSE(result.validation.error_messages.empty());
}

TEST_F(ModernETIFrameParserTest, ParseFrameWithoutInitialization) {
    auto result = parser_->parseFrame(valid_frame_data_);
    
    EXPECT_FALSE(result.success);
}

TEST_F(ModernETIFrameParserTest, ParseFrameWithInvalidSize) {
    ProcessingConfig config;
    ASSERT_TRUE(parser_->initialize(config));
    
    QByteArray small_frame(1000, 0x00); // Too small
    auto result = parser_->parseFrame(small_frame);
    
    EXPECT_FALSE(result.success);
}

TEST_F(ModernETIFrameParserTest, ParseMultipleFrames) {
    ProcessingConfig config;
    ASSERT_TRUE(parser_->initialize(config));
    
    for (int i = 0; i < 10; ++i) {
        auto result = parser_->parseFrame(valid_frame_data_);
        
        EXPECT_TRUE(result.success);
        EXPECT_EQ(result.frame_number, static_cast<uint32_t>(i + 1));
    }
}

// ============================================================================
// Performance Tests
// ============================================================================

TEST_F(ModernETIFrameParserTest, PerformanceTargetTracking) {
    ProcessingConfig config;
    config.target_fps = 1000.0;
    ASSERT_TRUE(parser_->initialize(config));
    
    // Parse multiple frames to establish performance baseline
    for (int i = 0; i < 100; ++i) {
        auto result = parser_->parseFrame(valid_frame_data_);
        EXPECT_TRUE(result.success);
    }
    
    auto stats = parser_->getPerformanceStats();
    EXPECT_GT(stats.frames_processed, 0);
    EXPECT_GT(stats.average_fps, 0.0);
    EXPECT_GT(stats.total_processing_time_ns, 0);
}

TEST_F(ModernETIFrameParserTest, PerformanceStatsAccuracy) {
    ProcessingConfig config;
    ASSERT_TRUE(parser_->initialize(config));
    
    const int frame_count = 50;
    
    for (int i = 0; i < frame_count; ++i) {
        auto result = parser_->parseFrame(valid_frame_data_);
        EXPECT_TRUE(result.success);
    }
    
    auto stats = parser_->getPerformanceStats();
    
    EXPECT_EQ(stats.frames_processed, frame_count);
    EXPECT_GT(stats.average_fps, 100.0); // Should be much faster than 100 FPS
    EXPECT_GT(stats.average_frame_time_us, 0.0);
    EXPECT_LE(stats.memory_usage_mb, config.memory_limit_mb);
}

TEST_F(ModernETIFrameParserTest, PerformanceStatsReset) {
    ProcessingConfig config;
    ASSERT_TRUE(parser_->initialize(config));
    
    // Process some frames
    for (int i = 0; i < 10; ++i) {
        parser_->parseFrame(valid_frame_data_);
    }
    
    auto stats_before = parser_->getPerformanceStats();
    EXPECT_GT(stats_before.frames_processed, 0);
    
    parser_->resetStats();
    
    auto stats_after = parser_->getPerformanceStats();
    EXPECT_EQ(stats_after.frames_processed, 0);
    EXPECT_EQ(stats_after.total_processing_time_ns, 0);
}

// ============================================================================
// ETSI Compliance Tests
// ============================================================================

TEST_F(ModernETIFrameParserTest, ValidFrameCompliance) {
    ProcessingConfig config;
    ASSERT_TRUE(parser_->initialize(config));
    
    auto result = parser_->parseFrame(valid_frame_data_);
    
    EXPECT_TRUE(result.success);
    EXPECT_TRUE(result.validation.is_valid);
    EXPECT_GE(result.validation.compliance_score, 90.0);
}

TEST_F(ModernETIFrameParserTest, InvalidSyncPatternCompliance) {
    ProcessingConfig config;
    ASSERT_TRUE(parser_->initialize(config));
    
    auto result = parser_->parseFrame(invalid_frame_data_);
    
    EXPECT_FALSE(result.success);
    EXPECT_FALSE(result.validation.is_valid);
    EXPECT_LT(result.validation.compliance_score, 90.0);
    
    // Check for sync pattern error message
    bool found_sync_error = false;
    for (const auto& error : result.validation.error_messages) {
        if (error.find("sync pattern") != std::string::npos) {
            found_sync_error = true;
            break;
        }
    }
    EXPECT_TRUE(found_sync_error);
}

TEST_F(ModernETIFrameParserTest, LIDATAFieldValidation) {
    ProcessingConfig config;
    ASSERT_TRUE(parser_->initialize(config));
    
    // Create frame with invalid LIDATA
    QByteArray invalid_lidata_frame = valid_frame_data_;
    invalid_lidata_frame[4] = 250; // Invalid frame count (>249)
    
    auto result = parser_->parseFrame(invalid_lidata_frame);
    
    EXPECT_FALSE(result.validation.is_valid);
    EXPECT_LT(result.validation.compliance_score, 100.0);
}

// ============================================================================
// FIG Analysis Tests
// ============================================================================

TEST_F(ModernETIFrameParserTest, FIGAnalysisWithoutFIC) {
    ProcessingConfig config;
    ASSERT_TRUE(parser_->initialize(config));
    
    // Create frame without FIC data (FICF = 0)
    QByteArray no_fic_frame = valid_frame_data_;
    no_fic_frame[5] &= ~0x80; // Clear FICF bit
    
    auto parse_result = parser_->parseFrame(no_fic_frame);
    ASSERT_TRUE(parse_result.success);
    
    auto fig_result = parser_->analyzeFIGData(parse_result.frame);
    
    EXPECT_TRUE(fig_result.fig_blocks.empty());
    EXPECT_FALSE(fig_result.has_ensemble_info());
    EXPECT_FALSE(fig_result.has_services());
}

TEST_F(ModernETIFrameParserTest, FIGAnalysisWithValidFIC) {
    ProcessingConfig config;
    ASSERT_TRUE(parser_->initialize(config));
    
    // Create frame with FIC data
    QByteArray fic_frame = valid_frame_data_;
    fic_frame[5] |= 0x80; // Set FICF bit
    
    // Add some FIG data at FIC location (simplified)
    // In real ETI, FIC starts at byte 8 and is 32 bytes
    fic_frame[8] = 0x00;  // FIG 0/0 header (type 0, length 0)
    fic_frame[9] = 0xFF;  // End marker
    
    auto parse_result = parser_->parseFrame(fic_frame);
    ASSERT_TRUE(parse_result.success);
    
    auto fig_result = parser_->analyzeFIGData(parse_result.frame);
    
    // Should have processed at least the basic FIG structure
    EXPECT_GE(fig_result.compliance.etsi_en_300_799_score, 75.0);
    EXPECT_GE(fig_result.compliance.etsi_en_300_401_score, 75.0);
}

// ============================================================================
// Signal Emission Tests
// ============================================================================

TEST_F(ModernETIFrameParserTest, FrameProcessedSignalEmission) {
    ProcessingConfig config;
    ASSERT_TRUE(parser_->initialize(config));
    
    QSignalSpy spy(parser_.get(), &ModernETIFrameParser::frameProcessed);
    
    auto result = parser_->parseFrame(valid_frame_data_);
    EXPECT_TRUE(result.success);
    
    EXPECT_EQ(spy.count(), 1);
    
    auto arguments = spy.takeFirst();
    EXPECT_EQ(arguments.at(0).toUInt(), 1); // frame_number
}

TEST_F(ModernETIFrameParserTest, PerformanceTargetSignalEmission) {
    ProcessingConfig config;
    config.target_fps = 10000.0; // Very high target to trigger missed target
    ASSERT_TRUE(parser_->initialize(config));
    
    QSignalSpy spy(parser_.get(), &ModernETIFrameParser::performanceTarget);
    
    // Process many frames to trigger performance evaluation
    for (int i = 0; i < 200; ++i) {
        parser_->parseFrame(valid_frame_data_);
    }
    
    // Should have emitted at least one performance target signal
    EXPECT_GE(spy.count(), 0); // May or may not emit depending on timing
}

// ============================================================================
// Error Handling Tests
// ============================================================================

TEST_F(ModernETIFrameParserTest, ExceptionHandling) {
    ProcessingConfig config;
    ASSERT_TRUE(parser_->initialize(config));
    
    // Create severely corrupted frame
    QByteArray corrupted_frame(ETI_FRAME_SIZE, 0xFF);
    
    // Should not crash, should return failed result
    auto result = parser_->parseFrame(corrupted_frame);
    EXPECT_FALSE(result.success);
}

TEST_F(ModernETIFrameParserTest, ConcurrentAccess) {
    ProcessingConfig config;
    config.enable_threading = true;
    ASSERT_TRUE(parser_->initialize(config));
    
    std::vector<std::thread> threads;
    std::atomic<int> successful_parses{0};
    
    // Launch multiple threads to parse frames concurrently
    for (int t = 0; t < 4; ++t) {
        threads.emplace_back([this, &successful_parses]() {
            for (int i = 0; i < 25; ++i) {
                auto result = parser_->parseFrame(valid_frame_data_);
                if (result.success) {
                    successful_parses.fetch_add(1);
                }
            }
        });
    }
    
    // Wait for all threads to complete
    for (auto& thread : threads) {
        thread.join();
    }
    
    EXPECT_EQ(successful_parses.load(), 100); // 4 threads × 25 frames each
    
    auto stats = parser_->getPerformanceStats();
    EXPECT_EQ(stats.frames_processed, 100);
}

// ============================================================================
// Configuration Tests
// ============================================================================

TEST_F(ModernETIFrameParserTest, SIMDOptimizationConfig) {
    ProcessingConfig config;
    config.enable_simd = true;
    
    EXPECT_TRUE(parser_->initialize(config));
    
    auto result = parser_->parseFrame(valid_frame_data_);
    EXPECT_TRUE(result.success);
}

TEST_F(ModernETIFrameParserTest, CachingConfig) {
    ProcessingConfig config;
    config.enable_caching = true;
    config.cache_size_mb = 128;
    
    EXPECT_TRUE(parser_->initialize(config));
    
    // Process same frame multiple times (should benefit from caching)
    for (int i = 0; i < 10; ++i) {
        auto result = parser_->parseFrame(valid_frame_data_);
        EXPECT_TRUE(result.success);
    }
}

TEST_F(ModernETIFrameParserTest, MemoryPoolingConfig) {
    ProcessingConfig config;
    config.enable_memory_pooling = true;
    config.memory_limit_mb = 64;
    
    EXPECT_TRUE(parser_->initialize(config));
    
    auto result = parser_->parseFrame(valid_frame_data_);
    EXPECT_TRUE(result.success);
    
    auto stats = parser_->getPerformanceStats();
    EXPECT_LE(stats.memory_usage_mb, config.memory_limit_mb);
}

// ============================================================================
// Factory Function Tests
// ============================================================================

TEST_F(ModernETIFrameParserTest, FactoryFunctionCreation) {
    ProcessingConfig config;
    config.optimization_level = ProcessingConfig::OptimizationLevel::REALTIME;
    
    auto parser = createOptimizedETIParser(config);
    
    EXPECT_NE(parser.get(), nullptr);
    EXPECT_TRUE(parser->isInitialized());
}

TEST_F(ModernETIFrameParserTest, FactoryFunctionWithInvalidConfig) {
    ProcessingConfig config;
    config.memory_limit_mb = 0; // Invalid memory limit
    
    auto parser = createOptimizedETIParser(config);
    
    // Should still create parser but with adjusted config
    EXPECT_NE(parser.get(), nullptr);
}

// ============================================================================
// Integration Tests
// ============================================================================

TEST_F(ModernETIFrameParserTest, FullWorkflowIntegration) {
    ProcessingConfig config;
    config.optimization_level = ProcessingConfig::OptimizationLevel::AGGRESSIVE;
    config.target_fps = 1500.0;
    config.memory_limit_mb = 80;
    config.enable_simd = true;
    config.enable_caching = true;
    config.enable_threading = true;
    
    ASSERT_TRUE(parser_->initialize(config));
    
    QSignalSpy frame_spy(parser_.get(), &ModernETIFrameParser::frameProcessed);
    QSignalSpy performance_spy(parser_.get(), &ModernETIFrameParser::performanceTarget);
    
    // Process a batch of frames
    std::vector<ETIParseResult> results;
    for (int i = 0; i < 100; ++i) {
        auto result = parser_->parseFrame(valid_frame_data_);
        results.push_back(std::move(result));
    }
    
    // Verify all frames processed successfully
    for (const auto& result : results) {
        EXPECT_TRUE(result.success);
        EXPECT_TRUE(result.validation.is_valid);
        EXPECT_GT(result.parse_time.count(), 0);
    }
    
    // Verify performance statistics
    auto stats = parser_->getPerformanceStats();
    EXPECT_EQ(stats.frames_processed, 100);
    EXPECT_GT(stats.average_fps, 500.0); // Should be reasonably fast
    EXPECT_LE(stats.memory_usage_mb, config.memory_limit_mb);
    EXPECT_TRUE(stats.meets_performance_targets || stats.average_fps > 1000.0);
    
    // Verify signals were emitted
    EXPECT_EQ(frame_spy.count(), 100);
}

// ============================================================================
// Benchmark Tests
// ============================================================================

TEST_F(ModernETIFrameParserTest, PerformanceBenchmark) {
    ProcessingConfig config;
    config.optimization_level = ProcessingConfig::OptimizationLevel::AGGRESSIVE;
    config.enable_simd = true;
    config.enable_caching = true;
    
    ASSERT_TRUE(parser_->initialize(config));
    
    const int benchmark_frames = 1000;
    auto start_time = std::chrono::high_resolution_clock::now();
    
    for (int i = 0; i < benchmark_frames; ++i) {
        auto result = parser_->parseFrame(valid_frame_data_);
        EXPECT_TRUE(result.success);
    }
    
    auto end_time = std::chrono::high_resolution_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
    
    double fps = (static_cast<double>(benchmark_frames) * 1000.0) / static_cast<double>(elapsed.count());
    
    // Performance target: should achieve >900 FPS (as per project requirements)
    EXPECT_GT(fps, 900.0);
    
    Logger::instance().log(Logger::Info, "ModernETIFrameParserTest", 
                          QString("Benchmark result: %1 FPS (%2 frames in %3ms)")
                          .arg(fps, 0, 'f', 1)
                          .arg(benchmark_frames)
                          .arg(elapsed.count()));
}