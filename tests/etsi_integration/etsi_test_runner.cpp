/**
 * @file etsi_test_runner.cpp
 * @brief ETSI Standards Compliance Test Runner
 * 
 * Comprehensive test runner for ETSI compliance framework with
 * automated test execution, performance validation, and
 * compliance reporting for professional broadcast applications.
 * 
 * @author Standards Compliance Agent
 * @date 2025
 * @copyright Copyright (c) 2025 ETI Stream Analyser Team
 * @version 1.0
 */

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "../../src/core/etsi/compliance_engine.h"
#include "../../src/core/etsi/alert_system.h"
#include "../../src/core/etsi/compliance_logger.h"
#include "../../src/core/etsi/realtime_monitor.h"
#include "../../src/core/eti_types.h"
#include "../fixtures/etsi_test_data/etsi_reference_data.h"
#include "../fixtures/eti_streams/etsi_compliant_frames.h"
#include <memory>
#include <chrono>
#include <vector>
#include <iostream>
#include <fstream>

using namespace etsi;
using namespace testing;

/**
 * @brief ETSI Integration Test Suite
 * 
 * Comprehensive integration tests for the complete ETSI compliance
 * framework validating real-world broadcast scenarios.
 */
class EtsiIntegrationTestSuite : public ::testing::Test {
protected:
    void SetUp() override {
        // Initialize test environment
        setup_test_environment();
        
        // Create core ETSI components
        setup_etsi_components();
        
        // Load test data
        load_test_data();
        
        // Verify test environment
        ASSERT_TRUE(verify_test_environment()) << "ETSI test environment setup failed";
    }
    
    void TearDown() override {
        // Cleanup components in reverse order
        realtime_monitor_.reset();
        logger_.reset();
        alert_system_.reset();
        compliance_engine_.reset();
        
        // Generate test report
        generate_test_report();
    }
    
    void setup_test_environment() {
        test_start_time_ = std::chrono::high_resolution_clock::now();
        
        // Create test directories
        std::filesystem::create_directories("./test_output/logs");
        std::filesystem::create_directories("./test_output/reports");
        std::filesystem::create_directories("./test_output/recordings");
    }
    
    void setup_etsi_components() {
        // Initialize compliance engine with broadcast quality settings
        compliance::EtsiComplianceEngine::Config engine_config;
        engine_config.target_level = compliance::ComplianceLevel::BROADCAST_QUALITY;
        engine_config.enable_real_time_monitoring = true;
        engine_config.enable_performance_monitoring = true;
        engine_config.enable_cross_standard_validation = true;
        
        compliance_engine_ = std::make_shared<compliance::EtsiComplianceEngine>(engine_config);
        ASSERT_TRUE(compliance_engine_->initialize()) << "Compliance engine initialization failed";
        
        // Initialize alert system with professional thresholds
        alerts::AlertSystemConfig alert_config;
        alert_config.enable_audio_monitoring = true;
        alert_config.enable_service_monitoring = true;
        alert_config.enable_emergency_alerts = true;
        alert_config.enable_performance_monitoring = true;
        
        alert_system_ = std::make_shared<alerts::EtsiAlertSystem>(alert_config);
        ASSERT_TRUE(alert_system_->initialize()) << "Alert system initialization failed";
        
        // Initialize compliance logger
        logging::AuditTrailConfig log_config;
        log_config.log_directory = "./test_output/logs";
        log_config.enable_audit_trail = true;
        log_config.enable_regulatory_documentation = true;
        log_config.enable_performance_logging = true;
        
        logger_ = std::make_shared<logging::EtsiComplianceLogger>(log_config);
        ASSERT_TRUE(logger_->initialize()) << "Compliance logger initialization failed";
        
        // Initialize real-time monitor
        realtime::EtsiRealtimeMonitor::Config monitor_config;
        monitor_config.mode = realtime::MonitoringMode::PERFORMANCE_TEST;
        monitor_config.max_concurrent_streams = 4;
        monitor_config.processing_threads = 2;
        monitor_config.compliance_level = compliance::ComplianceLevel::BROADCAST_QUALITY;
        monitor_config.enable_audio_monitoring = true;
        monitor_config.enable_performance_monitoring = true;
        
        realtime_monitor_ = std::make_shared<realtime::EtsiRealtimeMonitor>(monitor_config);
        
        // Connect components
        realtime_monitor_->set_compliance_engine(compliance_engine_);
        realtime_monitor_->set_alert_system(alert_system_);
        realtime_monitor_->set_logger(logger_);
        
        ASSERT_TRUE(realtime_monitor_->initialize()) << "Real-time monitor initialization failed";
    }
    
    void load_test_data() {
        // Load ETSI reference test data
        etsi_reference_data_ = std::make_unique<EtsiReferenceData>();
        ASSERT_TRUE(etsi_reference_data_->load()) << "Failed to load ETSI reference data";
        
        // Load compliant ETI frames
        etsi_compliant_frames_ = std::make_unique<EtsiCompliantFrames>();
        ASSERT_TRUE(etsi_compliant_frames_->load()) << "Failed to load compliant ETI frames";
        
        // Load test vectors
        test_vectors_ = load_etsi_test_vectors();
        ASSERT_FALSE(test_vectors_.empty()) << "No ETSI test vectors loaded";
    }
    
    bool verify_test_environment() {
        // Verify all components are properly initialized
        if (!compliance_engine_ || !compliance_engine_->supports_standard(compliance::EtsiStandard::EN_300_401)) {
            return false;
        }
        
        if (!alert_system_ || alert_system_->get_active_alerts().size() != 0) {
            return false;
        }
        
        if (!logger_) {
            return false;
        }
        
        if (!realtime_monitor_ || realtime_monitor_->is_monitoring()) {
            return false;
        }
        
        return true;
    }
    
    void generate_test_report() {
        auto test_end_time = std::chrono::high_resolution_clock::now();
        auto test_duration = std::chrono::duration_cast<std::chrono::milliseconds>(
            test_end_time - test_start_time_);
        
        std::ofstream report("./test_output/reports/etsi_integration_test_report.txt");
        report << "ETSI Integration Test Report\n";
        report << "============================\n\n";
        report << "Test Duration: " << test_duration.count() << " ms\n";
        
        if (compliance_engine_) {
            auto metrics = compliance_engine_->get_performance_metrics();
            report << "Compliance Engine Performance:\n";
            report << "  Total Validations: " << metrics.total_validations << "\n";
            report << "  Success Rate: " << (metrics.success_rate * 100.0) << "%\n";
            report << "  Avg Validation Time: " << metrics.avg_validation_time.count() << " μs\n";
        }
        
        if (realtime_monitor_) {
            auto stats = realtime_monitor_->get_statistics();
            report << "Real-time Monitor Statistics:\n";
            report << "  Frames Processed: " << stats.total_frames_processed << "\n";
            report << "  Violations Detected: " << stats.total_violations_detected << "\n";
            report << "  Alerts Generated: " << stats.total_alerts_generated << "\n";
        }
        
        report.close();
    }
    
    std::vector<EtiTestVector> load_etsi_test_vectors() {
        // Load standardized ETSI test vectors
        std::vector<EtiTestVector> vectors;
        
        // Add EN 300 401 test vectors
        vectors.push_back(create_en300401_test_vector());
        
        // Add EN 302 077 test vectors
        vectors.push_back(create_en302077_test_vector());
        
        // Add EN 300 799 test vectors
        vectors.push_back(create_en300799_test_vector());
        
        return vectors;
    }
    
    EtiTestVector create_en300401_test_vector() {
        EtiTestVector vector;
        vector.standard = compliance::EtsiStandard::EN_300_401;
        vector.description = "DAB Radio Broadcasting compliance test";
        vector.frame = etsi_compliant_frames_->get_dab_compliant_frame();
        vector.expected_compliance = true;
        vector.expected_violations = 0;
        return vector;
    }
    
    EtiTestVector create_en302077_test_vector() {
        EtiTestVector vector;
        vector.standard = compliance::EtsiStandard::EN_302_077;
        vector.description = "DAB+ Audio Coding compliance test";
        vector.frame = etsi_compliant_frames_->get_dabplus_compliant_frame();
        vector.expected_compliance = true;
        vector.expected_violations = 0;
        return vector;
    }
    
    EtiTestVector create_en300799_test_vector() {
        EtiTestVector vector;
        vector.standard = compliance::EtsiStandard::EN_300_799;
        vector.description = "ETI Distribution Interface compliance test";
        vector.frame = etsi_compliant_frames_->get_eti_compliant_frame();
        vector.expected_compliance = true;
        vector.expected_violations = 0;
        return vector;
    }
    
    // Test infrastructure
    std::chrono::high_resolution_clock::time_point test_start_time_;
    
    // Core ETSI components
    std::shared_ptr<compliance::EtsiComplianceEngine> compliance_engine_;
    std::shared_ptr<alerts::EtsiAlertSystem> alert_system_;
    std::shared_ptr<logging::EtsiComplianceLogger> logger_;
    std::shared_ptr<realtime::EtsiRealtimeMonitor> realtime_monitor_;
    
    // Test data
    std::unique_ptr<EtsiReferenceData> etsi_reference_data_;
    std::unique_ptr<EtsiCompliantFrames> etsi_compliant_frames_;
    std::vector<EtiTestVector> test_vectors_;
};

/**
 * @brief Test ETSI compliance engine integration
 */
TEST_F(EtsiIntegrationTestSuite, ComplianceEngineIntegration) {
    ASSERT_TRUE(compliance_engine_->supports_standard(compliance::EtsiStandard::EN_300_401));
    ASSERT_TRUE(compliance_engine_->supports_standard(compliance::EtsiStandard::EN_302_077));
    ASSERT_TRUE(compliance_engine_->supports_standard(compliance::EtsiStandard::EN_300_799));
    
    // Test frame validation
    for (const auto& test_vector : test_vectors_) {
        auto result = compliance_engine_->validate_eti_frame(test_vector.frame);
        
        EXPECT_EQ(result.is_compliant, test_vector.expected_compliance)
            << "Compliance mismatch for " << test_vector.description;
        
        EXPECT_EQ(result.violations.size(), test_vector.expected_violations)
            << "Violation count mismatch for " << test_vector.description;
        
        // Verify processing time is within professional limits (<100ms)
        EXPECT_LT(result.validation_time.count(), 100000)
            << "Validation time exceeds 100ms limit: " << result.validation_time.count() << "μs";
    }
}

/**
 * @brief Test alert system integration
 */
TEST_F(EtsiIntegrationTestSuite, AlertSystemIntegration) {
    // Generate test compliance result with violations
    compliance::ComplianceResult test_result;
    test_result.is_compliant = false;
    test_result.add_violation(compliance::ValidationIssue(
        compliance::EtsiStandard::EN_300_401,
        compliance::ValidationSeverity::ERROR,
        "Test violation for alert system",
        "Integration test"
    ));
    
    // Process result through alert system
    alert_system_->process_compliance_result(test_result);
    
    // Allow time for processing
    std::this_thread::sleep_for(std::chrono::milliseconds{100});
    
    // Verify alert was generated
    auto alerts = alert_system_->get_active_alerts();
    EXPECT_GT(alerts.size(), 0) << "No alerts generated for compliance violation";
    
    if (!alerts.empty()) {
        EXPECT_EQ(alerts[0].severity, alerts::AlertSeverity::WARNING);
        EXPECT_EQ(alerts[0].category, alerts::AlertCategory::ETSI_COMPLIANCE);
    }
}

/**
 * @brief Test compliance logger integration
 */
TEST_F(EtsiIntegrationTestSuite, ComplianceLoggerIntegration) {
    // Test logging compliance result
    compliance::ComplianceResult test_result;
    test_result.is_compliant = true;
    test_result.frames_analyzed = 1;
    test_result.set_compliance_percentage(compliance::EtsiStandard::EN_300_401, 100.0);
    
    logger_->log_compliance_result(test_result, "Integration test");
    
    // Force log flush
    logger_->flush_logs();
    
    // Verify log statistics
    auto stats = logger_->get_statistics();
    EXPECT_GT(stats.total_entries_logged, 0) << "No log entries recorded";
}

/**
 * @brief Test real-time monitor integration
 */
TEST_F(EtsiIntegrationTestSuite, RealtimeMonitorIntegration) {
    // Add test stream source
    realtime::StreamSource test_stream(1, "Test Stream", "test://localhost");
    test_stream.source_type = "file";
    
    ASSERT_TRUE(realtime_monitor_->add_stream_source(test_stream));
    
    // Test frame processing
    for (const auto& test_vector : test_vectors_) {
        auto result = realtime_monitor_->process_frame(test_vector.frame, 1);
        
        EXPECT_TRUE(result.frames_analyzed > 0) << "Frame was not processed";
        
        // Verify processing latency
        EXPECT_LT(result.validation_time.count(), 100000)
            << "Processing latency exceeds 100ms: " << result.validation_time.count() << "μs";
    }
    
    // Verify performance metrics
    auto metrics = realtime_monitor_->get_performance_metrics();
    EXPECT_GT(metrics.total_frames_processed, 0) << "No frames processed";
    EXPECT_LT(metrics.avg_processing_latency.count(), 100000) << "Average latency exceeds 100ms";
}

/**
 * @brief Test end-to-end ETSI compliance workflow
 */
TEST_F(EtsiIntegrationTestSuite, EndToEndComplianceWorkflow) {
    // Start real-time monitoring
    ASSERT_TRUE(realtime_monitor_->start_monitoring());
    
    // Process test frames through complete workflow
    size_t frames_processed = 0;
    size_t alerts_generated = 0;
    
    for (const auto& test_vector : test_vectors_) {
        // Process frame through real-time monitor
        auto result = realtime_monitor_->process_frame(test_vector.frame, 1);
        frames_processed++;
        
        // Check for generated alerts
        auto current_alerts = alert_system_->get_active_alerts();
        if (current_alerts.size() > alerts_generated) {
            alerts_generated = current_alerts.size();
        }
        
        // Allow processing time
        std::this_thread::sleep_for(std::chrono::milliseconds{10});
    }
    
    // Stop monitoring
    ASSERT_TRUE(realtime_monitor_->stop_monitoring());
    
    // Verify workflow execution
    EXPECT_EQ(frames_processed, test_vectors_.size()) << "Not all test frames were processed";
    
    // Verify performance compliance
    auto final_metrics = realtime_monitor_->get_performance_metrics();
    EXPECT_GT(final_metrics.total_frames_processed, 0) << "No frames processed in workflow";
    EXPECT_LT(final_metrics.avg_processing_latency.count(), 100000) 
        << "Average processing time exceeds 100ms professional requirement";
}

/**
 * @brief Test ETSI standards cross-validation
 */
TEST_F(EtsiIntegrationTestSuite, CrossStandardValidation) {
    // Enable cross-standard validation
    compliance_engine_->enable_standard_validation(compliance::EtsiStandard::EN_300_401, true);
    compliance_engine_->enable_standard_validation(compliance::EtsiStandard::EN_302_077, true);
    compliance_engine_->enable_standard_validation(compliance::EtsiStandard::EN_300_799, true);
    
    // Test with ensemble information
    auto test_frame = etsi_compliant_frames_->get_complete_ensemble_frame();
    auto ensemble = test_frame.extract_ensemble_info();
    
    auto result = compliance_engine_->validate_cross_standard_consistency(test_frame, ensemble);
    
    EXPECT_TRUE(result.standard_compliance.find(compliance::EtsiStandard::EN_300_401) != 
               result.standard_compliance.end()) << "EN 300 401 not validated";
    EXPECT_TRUE(result.standard_compliance.find(compliance::EtsiStandard::EN_300_799) != 
               result.standard_compliance.end()) << "EN 300 799 not validated";
    
    // Verify cross-standard consistency
    if (result.is_compliant) {
        EXPECT_GE(result.get_overall_compliance(), 95.0) << "Cross-standard compliance below 95%";
    }
}

/**
 * @brief Test performance under professional broadcast load
 */
TEST_F(EtsiIntegrationTestSuite, ProfessionalBroadcastPerformance) {
    // Configure for professional performance test
    const size_t PROFESSIONAL_FRAME_RATE = 250; // 250 frames/second (24ms intervals)
    const size_t TEST_DURATION_SECONDS = 5;
    const size_t TOTAL_FRAMES = PROFESSIONAL_FRAME_RATE * TEST_DURATION_SECONDS;
    
    // Start performance monitoring
    auto performance_start = std::chrono::high_resolution_clock::now();
    
    // Process frames at professional broadcast rate
    size_t successful_validations = 0;
    std::chrono::microseconds total_processing_time{0};
    
    for (size_t i = 0; i < TOTAL_FRAMES; ++i) {
        auto frame_start = std::chrono::high_resolution_clock::now();
        
        // Use rotating test vectors to simulate real broadcast content
        const auto& test_frame = test_vectors_[i % test_vectors_.size()].frame;
        
        auto result = compliance_engine_->validate_eti_frame(test_frame);
        
        auto frame_end = std::chrono::high_resolution_clock::now();
        auto frame_processing_time = std::chrono::duration_cast<std::chrono::microseconds>(
            frame_end - frame_start);
        
        total_processing_time += frame_processing_time;
        
        if (result.validation_time.count() < 100000) { // <100ms
            successful_validations++;
        }
        
        // Simulate 24ms frame interval (professional broadcast timing)
        std::this_thread::sleep_for(std::chrono::microseconds{24000});
    }
    
    auto performance_end = std::chrono::high_resolution_clock::now();
    auto total_test_time = std::chrono::duration_cast<std::chrono::milliseconds>(
        performance_end - performance_start);
    
    // Verify professional performance requirements
    double success_rate = static_cast<double>(successful_validations) / TOTAL_FRAMES * 100.0;
    double avg_processing_time = static_cast<double>(total_processing_time.count()) / TOTAL_FRAMES;
    double actual_frame_rate = static_cast<double>(TOTAL_FRAMES * 1000) / total_test_time.count();
    
    // Professional broadcast requirements validation
    EXPECT_GE(success_rate, 99.0) << "Success rate below 99% professional requirement: " << success_rate << "%";
    EXPECT_LT(avg_processing_time, 100000.0) << "Average processing time exceeds 100ms: " << avg_processing_time << "μs";
    EXPECT_GE(actual_frame_rate, 240.0) << "Frame processing rate below 240 fps: " << actual_frame_rate << " fps";
    
    std::cout << "Professional Performance Results:" << std::endl;
    std::cout << "  Success Rate: " << success_rate << "%" << std::endl;
    std::cout << "  Average Processing Time: " << avg_processing_time << "μs" << std::endl;
    std::cout << "  Frame Processing Rate: " << actual_frame_rate << " fps" << std::endl;
}

/**
 * @brief Main test runner entry point
 */
int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    
    std::cout << "Starting ETSI Standards Compliance Integration Tests" << std::endl;
    std::cout << "====================================================" << std::endl;
    
    // Configure test environment
    ::testing::GTEST_FLAG(color) = "yes";
    ::testing::GTEST_FLAG(print_time) = true;
    
    // Run all tests
    int result = RUN_ALL_TESTS();
    
    std::cout << std::endl;
    std::cout << "ETSI Integration Tests Complete" << std::endl;
    std::cout << "Test reports available in ./test_output/reports/" << std::endl;
    
    return result;
}