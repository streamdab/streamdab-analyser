/**
 * @file test_crc_error_recovery.cpp
 * @brief CRC Error Recovery Validation Tests
 * 
 * Comprehensive test suite for validating CRC error recovery mechanisms
 * that prevent violations while maintaining data integrity and compliance.
 * 
 * @author TDD Agent
 * @date 2025
 * @copyright Copyright (c) 2025 ETI Stream Analyser Team
 * @version 1.0
 */

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <vector>
#include <memory>
#include <random>
#include <algorithm>
#include <bitset>

// Core system headers
#include "../../src/core/etsi/alert_system.h"
#include "../../src/core/etsi/compliance_engine.h"
#include "../../src/core/eti_processor.h"

// Error correction headers
#include "../../src/core/error_correction/crc_calculator.h"
#include "../../src/core/error_correction/error_detector.h"
#include "../../src/core/error_correction/recovery_manager.h"

// Test fixtures
#include "../fixtures/eti_streams/optimized_compliance_frames.h"

using namespace etsi::alerts;
using namespace etsi::compliance;
using namespace error_correction;

using ::testing::_;
using ::testing::Return;
using ::testing::InSequence;

/**
 * @brief CRC error types for testing
 */
enum class CRCErrorType {
    SINGLE_BIT_ERROR,
    DOUBLE_BIT_ERROR,
    BURST_ERROR_SHORT,
    BURST_ERROR_LONG,
    MULTIPLE_RANDOM_ERRORS,
    SYSTEMATIC_ERROR,
    RECOVERABLE_PATTERN,
    UNRECOVERABLE_CORRUPTION
};

/**
 * @brief Error injection result
 */
struct ErrorInjectionResult {
    std::vector<uint8_t> original_data;
    std::vector<uint8_t> corrupted_data;
    CRCErrorType error_type;
    size_t error_count;
    std::vector<size_t> error_positions;
    uint16_t original_crc;
    uint16_t corrupted_crc;
    bool theoretically_recoverable;
};

/**
 * @brief Test fixture for CRC error recovery validation
 */
class CRCErrorRecoveryTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Configure alert system with error recovery enabled
        AlertSystemConfig config;
        config.enable_error_recovery = true;
        config.enable_crc_recovery = true;
        config.enable_forward_error_correction = true;
        config.max_recovery_attempts = 5;
        config.recovery_timeout_ms = 100;
        
        // Error recovery thresholds
        config.error_recovery.single_bit_recovery = true;
        config.error_recovery.burst_error_recovery = true;
        config.error_recovery.max_burst_length = 8; // bits
        config.error_recovery.enable_pattern_matching = true;
        config.error_recovery.enable_context_recovery = true;
        
        alert_system_ = std::make_unique<EtsiAlertSystem>(config);
        ASSERT_TRUE(alert_system_->initialize());
        
        // Configure compliance engine with error tolerance
        EtsiComplianceEngine::Config compliance_config;
        compliance_config.enable_error_tolerance = true;
        compliance_config.enable_recovery_bonus_system = true;
        compliance_config.max_recoverable_errors_per_frame = 3;
        
        compliance_engine_ = std::make_unique<EtsiComplianceEngine>(compliance_config);
        ASSERT_TRUE(compliance_engine_->initialize());
        
        // Configure CRC calculator
        CRCCalculatorConfig crc_config;
        crc_config.polynomial = 0x1021; // CRC-16-CCITT
        crc_config.initial_value = 0xFFFF;
        crc_config.final_xor = 0x0000;
        crc_config.reflect_input = false;
        crc_config.reflect_output = false;
        
        crc_calculator_ = std::make_unique<CRCCalculator>(crc_config);
        
        // Configure error detector
        ErrorDetectorConfig detector_config;
        detector_config.enable_syndrome_analysis = true;
        detector_config.enable_pattern_detection = true;
        detector_config.enable_burst_detection = true;
        
        error_detector_ = std::make_unique<ErrorDetector>(detector_config);
        ASSERT_TRUE(error_detector_->initialize());
        
        // Configure recovery manager
        RecoveryManagerConfig recovery_config;
        recovery_config.enable_aggressive_recovery = true;
        recovery_config.enable_context_based_recovery = true;
        recovery_config.max_iterations = 10;
        recovery_config.confidence_threshold = 0.8;
        
        recovery_manager_ = std::make_unique<RecoveryManager>(recovery_config);
        ASSERT_TRUE(recovery_manager_->initialize());
        
        // Initialize RNG for error injection
        rng_.seed(12345); // Deterministic for reproducible tests
    }
    
    void TearDown() override {
        std::cout << "CRC error recovery test completed" << std::endl;
        
        // Print recovery statistics
        if (recovery_manager_) {
            auto stats = recovery_manager_->get_recovery_statistics();
            std::cout << "Recovery Statistics:" << std::endl;
            std::cout << "  Total attempts: " << stats.total_recovery_attempts << std::endl;
            std::cout << "  Successful recoveries: " << stats.successful_recoveries << std::endl;
            std::cout << "  Success rate: " << (stats.success_rate * 100.0) << "%" << std::endl;
        }
    }
    
    /**
     * @brief Inject specific type of CRC error into data
     */
    ErrorInjectionResult InjectCRCError(const std::vector<uint8_t>& data, CRCErrorType error_type) {
        ErrorInjectionResult result;
        result.original_data = data;
        result.corrupted_data = data;
        result.error_type = error_type;
        result.error_count = 0;
        result.theoretically_recoverable = false;
        
        // Calculate original CRC
        result.original_crc = crc_calculator_->calculate(data.data(), data.size());
        
        switch (error_type) {
            case CRCErrorType::SINGLE_BIT_ERROR:
                inject_single_bit_error(result);
                result.theoretically_recoverable = true;
                break;
                
            case CRCErrorType::DOUBLE_BIT_ERROR:
                inject_double_bit_error(result);
                result.theoretically_recoverable = true;
                break;
                
            case CRCErrorType::BURST_ERROR_SHORT:
                inject_burst_error(result, 3); // 3-bit burst
                result.theoretically_recoverable = true;
                break;
                
            case CRCErrorType::BURST_ERROR_LONG:
                inject_burst_error(result, 7); // 7-bit burst
                result.theoretically_recoverable = true;
                break;
                
            case CRCErrorType::MULTIPLE_RANDOM_ERRORS:
                inject_multiple_random_errors(result, 2); // 2 random errors
                result.theoretically_recoverable = true;
                break;
                
            case CRCErrorType::SYSTEMATIC_ERROR:
                inject_systematic_error(result);
                result.theoretically_recoverable = true;
                break;
                
            case CRCErrorType::RECOVERABLE_PATTERN:
                inject_recoverable_pattern(result);
                result.theoretically_recoverable = true;
                break;
                
            case CRCErrorType::UNRECOVERABLE_CORRUPTION:
                inject_unrecoverable_corruption(result);
                result.theoretically_recoverable = false;
                break;
        }
        
        // Calculate corrupted CRC
        result.corrupted_crc = crc_calculator_->calculate(result.corrupted_data.data(), 
                                                        result.corrupted_data.size());
        
        return result;
    }
    
    /**
     * @brief Test recovery for a specific error injection result
     */
    bool TestRecovery(const ErrorInjectionResult& error_result) {
        // Attempt recovery
        auto recovery_result = recovery_manager_->attempt_recovery(
            error_result.corrupted_data,
            error_result.original_crc,
            error_result.error_type
        );
        
        if (!recovery_result.success) {
            return false;
        }
        
        // Verify recovery by comparing with original
        return (recovery_result.recovered_data == error_result.original_data);
    }
    
private:
    void inject_single_bit_error(ErrorInjectionResult& result) {
        if (result.corrupted_data.empty()) return;
        
        // Choose random byte and bit position
        std::uniform_int_distribution<size_t> byte_dist(0, result.corrupted_data.size() - 1);
        std::uniform_int_distribution<int> bit_dist(0, 7);
        
        size_t byte_pos = byte_dist(rng_);
        int bit_pos = bit_dist(rng_);
        
        // Flip the bit
        result.corrupted_data[byte_pos] ^= (1 << bit_pos);
        
        result.error_count = 1;
        result.error_positions.push_back(byte_pos * 8 + bit_pos);
    }
    
    void inject_double_bit_error(ErrorInjectionResult& result) {
        if (result.corrupted_data.size() < 2) return;
        
        std::uniform_int_distribution<size_t> byte_dist(0, result.corrupted_data.size() - 1);
        std::uniform_int_distribution<int> bit_dist(0, 7);
        
        // First error
        size_t byte_pos1 = byte_dist(rng_);
        int bit_pos1 = bit_dist(rng_);
        result.corrupted_data[byte_pos1] ^= (1 << bit_pos1);
        result.error_positions.push_back(byte_pos1 * 8 + bit_pos1);
        
        // Second error (ensure different position)
        size_t byte_pos2, bit_pos2;
        do {
            byte_pos2 = byte_dist(rng_);
            bit_pos2 = bit_dist(rng_);
        } while (byte_pos1 == byte_pos2 && bit_pos1 == bit_pos2);
        
        result.corrupted_data[byte_pos2] ^= (1 << bit_pos2);
        result.error_positions.push_back(byte_pos2 * 8 + bit_pos2);
        
        result.error_count = 2;
    }
    
    void inject_burst_error(ErrorInjectionResult& result, size_t burst_length) {
        if (result.corrupted_data.empty() || burst_length == 0) return;
        
        // Choose random starting position
        size_t max_start_bit = result.corrupted_data.size() * 8 - burst_length;
        std::uniform_int_distribution<size_t> start_dist(0, max_start_bit);
        size_t start_bit = start_dist(rng_);
        
        // Flip consecutive bits
        for (size_t i = 0; i < burst_length; ++i) {
            size_t bit_pos = start_bit + i;
            size_t byte_pos = bit_pos / 8;
            int bit_offset = bit_pos % 8;
            
            result.corrupted_data[byte_pos] ^= (1 << bit_offset);
            result.error_positions.push_back(bit_pos);
        }
        
        result.error_count = burst_length;
    }
    
    void inject_multiple_random_errors(ErrorInjectionResult& result, size_t error_count) {
        for (size_t i = 0; i < error_count; ++i) {
            inject_single_bit_error(result);
        }
        result.error_count = error_count;
    }
    
    void inject_systematic_error(ErrorInjectionResult& result) {
        // Inject error pattern that affects specific data structures
        if (result.corrupted_data.size() >= 8) {
            // Corrupt what might be a length field (first 2 bytes)
            result.corrupted_data[0] ^= 0x01;
            result.corrupted_data[1] ^= 0x02;
            result.error_count = 2;
            result.error_positions.push_back(0);
            result.error_positions.push_back(8);
        }
    }
    
    void inject_recoverable_pattern(ErrorInjectionResult& result) {
        // Inject a known recoverable pattern (alternating bits)
        if (result.corrupted_data.size() >= 4) {
            result.corrupted_data[0] ^= 0xAA; // 10101010
            result.corrupted_data[1] ^= 0x55; // 01010101
            result.error_count = 8; // 8 bit flips total
            for (int i = 0; i < 8; ++i) {
                result.error_positions.push_back(i);
                result.error_positions.push_back(8 + i);
            }
        }
    }
    
    void inject_unrecoverable_corruption(ErrorInjectionResult& result) {
        // Corrupt large portion of data
        std::uniform_int_distribution<uint8_t> byte_dist(0, 255);
        
        size_t corruption_size = std::min(result.corrupted_data.size() / 2, size_t(16));
        for (size_t i = 0; i < corruption_size; ++i) {
            result.corrupted_data[i] = byte_dist(rng_);
            result.error_count++;
        }
    }
    
    std::unique_ptr<EtsiAlertSystem> alert_system_;
    std::unique_ptr<EtsiComplianceEngine> compliance_engine_;
    std::unique_ptr<CRCCalculator> crc_calculator_;
    std::unique_ptr<ErrorDetector> error_detector_;
    std::unique_ptr<RecoveryManager> recovery_manager_;
    std::mt19937 rng_;
};

/**
 * @brief Test single bit error recovery
 */
TEST_F(CRCErrorRecoveryTest, testSingleBitErrorRecovery) {
    // Create test ETI frame
    auto test_frame = fixtures::eti::CreatePerfectComplianceFrame();
    auto frame_data = test_frame.to_byte_vector();
    
    // Inject single bit error
    auto error_result = InjectCRCError(frame_data, CRCErrorType::SINGLE_BIT_ERROR);
    
    // Attempt recovery
    bool recovery_successful = TestRecovery(error_result);
    
    // VALIDATE: Single bit errors should be recoverable
    EXPECT_TRUE(recovery_successful) 
        << "Single bit error should be recoverable";
    
    // Test with alert system
    EtiFrame corrupted_frame = test_frame;
    corrupted_frame.set_data(error_result.corrupted_data);
    corrupted_frame.error_flags |= ETI_ERROR_FLAG_FIC_CRC_ERROR;
    
    bool correction_attempted = alert_system_->attempt_error_correction(corrupted_frame);
    EXPECT_TRUE(correction_attempted);
    
    auto alerts = alert_system_->check_error_correction_thresholds(corrupted_frame);
    
    // Should have recovery notification, not error alert
    bool has_recovery_info = false;
    bool has_error_warning = false;
    
    for (const auto& alert : alerts) {
        if (alert.category == AlertCategory::ERROR_CORRECTION) {
            if (alert.severity == AlertSeverity::INFO && 
                alert.message.find("recovered") != std::string::npos) {
                has_recovery_info = true;
            }
            if (alert.severity >= AlertSeverity::WARNING) {
                has_error_warning = true;
            }
        }
    }
    
    EXPECT_TRUE(has_recovery_info) << "Should have recovery notification";
    EXPECT_FALSE(has_error_warning) << "Should not have error warning for recovered error";
}

/**
 * @brief Test double bit error recovery
 */
TEST_F(CRCErrorRecoveryTest, testDoubleBitErrorRecovery) {
    auto test_frame = fixtures::eti::CreatePerfectComplianceFrame();
    auto frame_data = test_frame.to_byte_vector();
    
    // Inject double bit error
    auto error_result = InjectCRCError(frame_data, CRCErrorType::DOUBLE_BIT_ERROR);
    
    // Attempt recovery
    bool recovery_successful = TestRecovery(error_result);
    
    // VALIDATE: Double bit errors should be recoverable with advanced techniques
    EXPECT_TRUE(recovery_successful) 
        << "Double bit error should be recoverable with advanced error correction";
    
    // Test compliance impact
    EtiFrame corrupted_frame = test_frame;
    corrupted_frame.set_data(error_result.corrupted_data);
    corrupted_frame.error_flags |= ETI_ERROR_FLAG_FIC_CRC_ERROR;
    
    // Enable error recovery in compliance engine
    auto result_without_recovery = compliance_engine_->validate_eti_frame(corrupted_frame);
    
    // Enable recovery
    compliance_engine_->enable_error_recovery(true);
    auto result_with_recovery = compliance_engine_->validate_eti_frame(corrupted_frame);
    
    // Recovery should improve compliance score
    EXPECT_GE(result_with_recovery.get_overall_compliance(), 
             result_without_recovery.get_overall_compliance());
    
    std::cout << "Double bit error recovery:" << std::endl;
    std::cout << "  Without recovery: " << result_without_recovery.get_overall_compliance() << "%" << std::endl;
    std::cout << "  With recovery: " << result_with_recovery.get_overall_compliance() << "%" << std::endl;
}

/**
 * @brief Test burst error recovery
 */
TEST_F(CRCErrorRecoveryTest, testBurstErrorRecovery) {
    auto test_frame = fixtures::eti::CreatePerfectComplianceFrame();
    auto frame_data = test_frame.to_byte_vector();
    
    // Test both short and long burst errors
    std::vector<CRCErrorType> burst_types = {
        CRCErrorType::BURST_ERROR_SHORT,
        CRCErrorType::BURST_ERROR_LONG
    };
    
    for (auto burst_type : burst_types) {
        auto error_result = InjectCRCError(frame_data, burst_type);
        bool recovery_successful = TestRecovery(error_result);
        
        // Short bursts should always be recoverable, long bursts should often be recoverable
        if (burst_type == CRCErrorType::BURST_ERROR_SHORT) {
            EXPECT_TRUE(recovery_successful) 
                << "Short burst error should be recoverable";
        } else {
            // Long bursts are harder but should still often succeed
            std::cout << "Long burst error recovery: " 
                      << (recovery_successful ? "SUCCESS" : "FAILED") << std::endl;
        }
        
        // Test alert handling
        EtiFrame corrupted_frame = test_frame;
        corrupted_frame.set_data(error_result.corrupted_data);
        corrupted_frame.error_flags |= ETI_ERROR_FLAG_BURST_ERROR;
        
        bool correction_attempted = alert_system_->attempt_error_correction(corrupted_frame);
        EXPECT_TRUE(correction_attempted);
        
        auto alerts = alert_system_->check_error_correction_thresholds(corrupted_frame);
        
        // Should attempt burst error recovery
        bool has_burst_recovery_attempt = false;
        for (const auto& alert : alerts) {
            if (alert.message.find("burst") != std::string::npos) {
                has_burst_recovery_attempt = true;
                break;
            }
        }
        EXPECT_TRUE(has_burst_recovery_attempt) << "Should attempt burst error recovery";
    }
}

/**
 * @brief Test multiple error recovery
 */
TEST_F(CRCErrorRecoveryTest, testMultipleErrorRecovery) {
    auto test_frame = fixtures::eti::CreatePerfectComplianceFrame();
    auto frame_data = test_frame.to_byte_vector();
    
    // Inject multiple random errors
    auto error_result = InjectCRCError(frame_data, CRCErrorType::MULTIPLE_RANDOM_ERRORS);
    
    // Attempt recovery
    bool recovery_successful = TestRecovery(error_result);
    
    // Multiple errors are harder to recover but should be attempted
    std::cout << "Multiple error recovery: " 
              << (recovery_successful ? "SUCCESS" : "FAILED") << std::endl;
    
    // Test recovery statistics
    auto recovery_stats = recovery_manager_->get_recovery_statistics();
    EXPECT_GT(recovery_stats.total_recovery_attempts, 0);
    
    // Test with compliance engine
    EtiFrame corrupted_frame = test_frame;
    corrupted_frame.set_data(error_result.corrupted_data);
    corrupted_frame.error_flags |= ETI_ERROR_FLAG_MULTIPLE_ERRORS;
    
    compliance_engine_->enable_error_recovery(true);
    auto result = compliance_engine_->validate_eti_frame(corrupted_frame);
    
    // Should get recovery bonus even if not fully successful
    if (recovery_successful) {
        EXPECT_GT(result.get_recovery_bonus(), 0.0);
        EXPECT_GE(result.get_overall_compliance(), 95.0); // High compliance with successful recovery
    } else {
        // Even failed recovery attempts should provide some benefit
        EXPECT_GE(result.get_overall_compliance(), 80.0); // Reasonable compliance with attempted recovery
    }
}

/**
 * @brief Test systematic error pattern recovery
 */
TEST_F(CRCErrorRecoveryTest, testSystematicErrorPatternRecovery) {
    auto test_frame = fixtures::eti::CreatePerfectComplianceFrame();
    auto frame_data = test_frame.to_byte_vector();
    
    // Inject systematic error
    auto error_result = InjectCRCError(frame_data, CRCErrorType::SYSTEMATIC_ERROR);
    
    // Enable pattern-based recovery
    recovery_manager_->enable_pattern_based_recovery(true);
    
    // Attempt recovery
    bool recovery_successful = TestRecovery(error_result);
    
    // Systematic errors should be recoverable with pattern matching
    EXPECT_TRUE(recovery_successful) 
        << "Systematic error should be recoverable with pattern matching";
    
    // Test context-aware recovery
    recovery_manager_->set_context_data(frame_data); // Provide context
    bool context_recovery_successful = TestRecovery(error_result);
    
    EXPECT_TRUE(context_recovery_successful) 
        << "Context-aware recovery should succeed for systematic errors";
    
    std::cout << "Systematic error recovery:" << std::endl;
    std::cout << "  Standard recovery: " << (recovery_successful ? "SUCCESS" : "FAILED") << std::endl;
    std::cout << "  Context-aware recovery: " << (context_recovery_successful ? "SUCCESS" : "FAILED") << std::endl;
}

/**
 * @brief Test recoverable pattern recognition
 */
TEST_F(CRCErrorRecoveryTest, testRecoverablePatternRecognition) {
    auto test_frame = fixtures::eti::CreatePerfectComplianceFrame();
    auto frame_data = test_frame.to_byte_vector();
    
    // Inject known recoverable pattern
    auto error_result = InjectCRCError(frame_data, CRCErrorType::RECOVERABLE_PATTERN);
    
    // Test pattern detection
    auto pattern_analysis = error_detector_->analyze_error_pattern(
        error_result.original_data, 
        error_result.corrupted_data
    );
    
    EXPECT_TRUE(pattern_analysis.is_recoverable_pattern);
    EXPECT_GT(pattern_analysis.recovery_confidence, 0.8); // High confidence
    
    // Attempt recovery with pattern recognition
    recovery_manager_->enable_pattern_recognition(true);
    bool recovery_successful = TestRecovery(error_result);
    
    EXPECT_TRUE(recovery_successful) 
        << "Recoverable pattern should be successfully recovered";
    
    // Test alert classification
    EtiFrame corrupted_frame = test_frame;
    corrupted_frame.set_data(error_result.corrupted_data);
    corrupted_frame.error_flags |= ETI_ERROR_FLAG_PATTERN_ERROR;
    
    auto alerts = alert_system_->check_error_correction_thresholds(corrupted_frame);
    
    bool has_pattern_recognition_alert = false;
    for (const auto& alert : alerts) {
        if (alert.message.find("pattern") != std::string::npos && 
            alert.severity == AlertSeverity::INFO) {
            has_pattern_recognition_alert = true;
            break;
        }
    }
    EXPECT_TRUE(has_pattern_recognition_alert) << "Should detect recoverable pattern";
}

/**
 * @brief Test unrecoverable error handling
 */
TEST_F(CRCErrorRecoveryTest, testUnrecoverableErrorHandling) {
    auto test_frame = fixtures::eti::CreatePerfectComplianceFrame();
    auto frame_data = test_frame.to_byte_vector();
    
    // Inject unrecoverable corruption
    auto error_result = InjectCRCError(frame_data, CRCErrorType::UNRECOVERABLE_CORRUPTION);
    
    // Attempt recovery
    bool recovery_successful = TestRecovery(error_result);
    
    // Should fail to recover
    EXPECT_FALSE(recovery_successful) 
        << "Unrecoverable corruption should not be recoverable";
    
    // Test graceful failure handling
    EtiFrame corrupted_frame = test_frame;
    corrupted_frame.set_data(error_result.corrupted_data);
    corrupted_frame.error_flags |= ETI_ERROR_FLAG_UNRECOVERABLE;
    
    bool correction_attempted = alert_system_->attempt_error_correction(corrupted_frame);
    EXPECT_TRUE(correction_attempted); // Should still attempt
    
    auto alerts = alert_system_->check_error_correction_thresholds(corrupted_frame);
    
    // Should have critical error alert
    bool has_critical_error = false;
    for (const auto& alert : alerts) {
        if (alert.severity == AlertSeverity::CRITICAL && 
            alert.message.find("unrecoverable") != std::string::npos) {
            has_critical_error = true;
            break;
        }
    }
    EXPECT_TRUE(has_critical_error) << "Should alert for unrecoverable error";
    
    // Test compliance impact
    compliance_engine_->enable_error_recovery(true);
    auto result = compliance_engine_->validate_eti_frame(corrupted_frame);
    
    // Should still have reasonable compliance due to attempted recovery
    EXPECT_GE(result.get_overall_compliance(), 50.0); // At least 50% for attempting recovery
    EXPECT_LT(result.get_overall_compliance(), 80.0);  // But not too high for unrecoverable error
}

/**
 * @brief Test recovery performance under load
 */
TEST_F(CRCErrorRecoveryTest, testRecoveryPerformanceUnderLoad) {
    auto test_frame = fixtures::eti::CreatePerfectComplianceFrame();
    auto frame_data = test_frame.to_byte_vector();
    
    std::vector<CRCErrorType> error_types = {
        CRCErrorType::SINGLE_BIT_ERROR,
        CRCErrorType::DOUBLE_BIT_ERROR,
        CRCErrorType::BURST_ERROR_SHORT,
        CRCErrorType::MULTIPLE_RANDOM_ERRORS
    };
    
    auto start_time = std::chrono::high_resolution_clock::now();
    
    int total_tests = 100;
    int successful_recoveries = 0;
    std::vector<std::chrono::microseconds> recovery_times;
    
    for (int i = 0; i < total_tests; ++i) {
        CRCErrorType error_type = error_types[i % error_types.size()];
        auto error_result = InjectCRCError(frame_data, error_type);
        
        auto recovery_start = std::chrono::high_resolution_clock::now();
        bool recovery_successful = TestRecovery(error_result);
        auto recovery_end = std::chrono::high_resolution_clock::now();
        
        auto recovery_time = std::chrono::duration_cast<std::chrono::microseconds>(
            recovery_end - recovery_start);
        recovery_times.push_back(recovery_time);
        
        if (recovery_successful) {
            successful_recoveries++;
        }
    }
    
    auto end_time = std::chrono::high_resolution_clock::now();
    auto total_time = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
    
    // Calculate statistics
    auto avg_recovery_time = std::accumulate(recovery_times.begin(), recovery_times.end(),
                                           std::chrono::microseconds(0)) / recovery_times.size();
    auto max_recovery_time = *std::max_element(recovery_times.begin(), recovery_times.end());
    double success_rate = static_cast<double>(successful_recoveries) / total_tests;
    
    // VALIDATE: Performance requirements
    EXPECT_LT(avg_recovery_time.count(), 50000); // <50ms average recovery time
    EXPECT_LT(max_recovery_time.count(), 100000); // <100ms maximum recovery time
    EXPECT_GT(success_rate, 0.8); // >80% success rate
    
    std::cout << "Recovery Performance Results (" << total_tests << " tests):" << std::endl;
    std::cout << "  Success rate: " << (success_rate * 100.0) << "%" << std::endl;
    std::cout << "  Average recovery time: " << avg_recovery_time.count() << " μs" << std::endl;
    std::cout << "  Maximum recovery time: " << max_recovery_time.count() << " μs" << std::endl;
    std::cout << "  Total test time: " << total_time.count() << " ms" << std::endl;
}

/**
 * @brief Test recovery with compliance bonus system
 */
TEST_F(CRCErrorRecoveryTest, testRecoveryComplianceBonusSystem) {
    auto test_frame = fixtures::eti::CreatePerfectComplianceFrame();
    auto frame_data = test_frame.to_byte_vector();
    
    // Test different error types and their compliance impact
    std::vector<std::pair<CRCErrorType, double>> error_tests = {
        {CRCErrorType::SINGLE_BIT_ERROR, 98.0},      // Should achieve high compliance
        {CRCErrorType::DOUBLE_BIT_ERROR, 95.0},      // Good compliance with recovery
        {CRCErrorType::BURST_ERROR_SHORT, 92.0},     // Reasonable compliance
        {CRCErrorType::MULTIPLE_RANDOM_ERRORS, 88.0} // Lower but acceptable compliance
    };
    
    for (const auto& test_case : error_tests) {
        auto error_result = InjectCRCError(frame_data, test_case.first);
        
        EtiFrame corrupted_frame = test_frame;
        corrupted_frame.set_data(error_result.corrupted_data);
        corrupted_frame.error_flags |= ETI_ERROR_FLAG_FIC_CRC_ERROR;
        
        // Test without recovery
        compliance_engine_->enable_error_recovery(false);
        auto result_without_recovery = compliance_engine_->validate_eti_frame(corrupted_frame);
        
        // Test with recovery
        compliance_engine_->enable_error_recovery(true);
        auto result_with_recovery = compliance_engine_->validate_eti_frame(corrupted_frame);
        
        // Recovery should improve compliance
        EXPECT_GT(result_with_recovery.get_overall_compliance(), 
                 result_without_recovery.get_overall_compliance());
        
        // Should achieve expected minimum compliance with recovery
        if (TestRecovery(error_result)) {
            EXPECT_GE(result_with_recovery.get_overall_compliance(), test_case.second);
            EXPECT_GT(result_with_recovery.get_recovery_bonus(), 0.0);
        }
        
        std::cout << "Error type " << static_cast<int>(test_case.first) << ":" << std::endl;
        std::cout << "  Without recovery: " << result_without_recovery.get_overall_compliance() << "%" << std::endl;
        std::cout << "  With recovery: " << result_with_recovery.get_overall_compliance() << "%" << std::endl;
        std::cout << "  Recovery bonus: " << result_with_recovery.get_recovery_bonus() << "%" << std::endl;
    }
}

/**
 * @brief Main test execution
 */
int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    
    std::cout << "\n🔧 CRC ERROR RECOVERY VALIDATION TESTS" << std::endl;
    std::cout << "======================================" << std::endl;
    std::cout << "Testing CRC error recovery mechanisms and compliance impact..." << std::endl;
    
    auto start_time = std::chrono::high_resolution_clock::now();
    int result = RUN_ALL_TESTS();
    auto end_time = std::chrono::high_resolution_clock::now();
    
    auto total_time = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
    
    std::cout << "\n📊 CRC RECOVERY TEST SUMMARY" << std::endl;
    std::cout << "============================" << std::endl;
    std::cout << "Total execution time: " << total_time.count() << " ms" << std::endl;
    
    if (result == 0) {
        std::cout << "\n✅ ALL CRC RECOVERY TESTS PASSED - ERROR RECOVERY VALIDATED" << std::endl;
        std::cout << "🛡️ DATA INTEGRITY PROTECTION SYSTEMS OPERATIONAL" << std::endl;
    } else {
        std::cout << "\n❌ CRC RECOVERY TESTS FAILED - REQUIRES ATTENTION" << std::endl;
    }
    
    return result;
}