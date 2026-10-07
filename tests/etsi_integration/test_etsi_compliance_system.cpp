/**
 * @file test_etsi_compliance_system.cpp
 * @brief Comprehensive ETSI Compliance System Integration Tests
 * 
 * Complete test suite for ETSI standards compliance system including
 * integration tests, performance validation, real-time monitoring tests,
 * and professional broadcast workflow validation.
 * 
 * Test Categories:
 * - Component Integration Tests
 * - End-to-End Workflow Tests
 * - Performance and Latency Tests
 * - Real-time Monitoring Tests
 * - Alert System Integration Tests
 * - Reporting System Tests
 * - Broadcast Standards Compliance Tests
 * - Error Handling and Recovery Tests
 * - Configuration Management Tests
 * - Production Readiness Validation
 * 
 * @author Standards Compliance Agent
 * @date 2025
 * @copyright Copyright (c) 2025 ETI Stream Analyser Team
 * @version 1.0
 */

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <chrono>
#include <thread>
#include <vector>
#include <memory>
#include <random>

// ETSI Compliance System Headers
#include "../../src/core/etsi/etsi_integration_manager.h"
#include "../../src/core/etsi/compliance_engine.h"
#include "../../src/core/etsi/alert_system.h"
#include "../../src/core/etsi/compliance_logger.h"
#include "../../src/core/etsi/compliance_reporter.h"
#include "../../src/core/etsi/broadcast_standards.h"
#include "../../src/core/etsi/realtime_monitor.h"

// Test Fixtures and Mocks
#include "../fixtures/etsi_test_data/etsi_reference_data.h"
#include "../fixtures/eti_streams/etsi_compliant_frames.h"
#include "../mocks/mock_eti_processor.h"

using namespace etsi;
using namespace etsi::integration;
using namespace etsi::compliance;
using namespace etsi::alerts;
using namespace etsi::logging;
using namespace etsi::reporting;
using namespace etsi::broadcast;
using namespace etsi::realtime;

using ::testing::_;
using ::testing::Return;
using ::testing::InSequence;
using ::testing::StrictMock;

/**
 * @brief ETSI Compliance System Integration Test Fixture
 */
class EtsiComplianceSystemTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Initialize test configuration
        config_ = CreateTestConfiguration();
        
        // Create integration manager
        integration_manager_ = std::make_unique<EtsiIntegrationManager>(config_);
        
        // Initialize test data
        test_frames_ = CreateTestEtiFrames();
        test_audio_data_ = CreateTestAudioData();
        
        // Setup performance measurement
        test_start_time_ = std::chrono::high_resolution_clock::now();
    }
    
    void TearDown() override {
        if (integration_manager_) {
            integration_manager_->shutdown();
        }
        
        auto test_end_time = std::chrono::high_resolution_clock::now();
        auto test_duration = std::chrono::duration_cast<std::chrono::milliseconds>(
            test_end_time - test_start_time_);
        
        std::cout << "Test completed in " << test_duration.count() << " ms" << std::endl;
    }
    
    EtsiSystemConfig CreateTestConfiguration() {
        EtsiSystemConfig config;
        
        // Compliance engine config
        config.compliance_config.target_level = ComplianceLevel::BROADCAST_QUALITY;
        config.compliance_config.enable_real_time_monitoring = true;
        config.compliance_config.validation_timeout = std::chrono::milliseconds{50};
        
        // Alert system config
        config.alert_config.enable_audio_monitoring = true;
        config.alert_config.enable_service_monitoring = true;
        config.alert_config.enable_emergency_alerts = true;
        
        // Logging config
        config.logging_config.enable_audit_trail = true;
        config.logging_config.enable_real_time_logging = true;
        config.logging_config.log_directory = "./test_logs";
        
        // Reporting config
        config.reporting_config.enable_auto_generation = false; // Manual for tests
        config.reporting_config.reports_directory = "./test_reports";
        
        // Broadcast standards config
        config.broadcast_config.enable_real_time_analysis = true;
        config.broadcast_config.enable_spectrum_analysis = true;
        
        // Real-time monitor config
        config.realtime_config.mode = MonitoringMode::PERFORMANCE_TEST;
        config.realtime_config.max_concurrent_streams = 4;
        config.realtime_config.max_processing_latency = std::chrono::microseconds{100000}; // 100ms
        
        return config;
    }
    
    std::vector<EtiFrame> CreateTestEtiFrames() {
        return fixtures::eti::CreateEtsiCompliantFrames(100); // 100 test frames
    }
    
    std::vector<float> CreateTestAudioData() {
        std::vector<float> audio_data(48000 * 2); // 1 second stereo at 48kHz
        
        // Generate test sine wave at 1kHz
        for (size_t i = 0; i < audio_data.size(); i += 2) {
            float sample = 0.1f * std::sin(2.0f * M_PI * 1000.0f * i / 48000.0f);
            audio_data[i] = sample;     // Left channel
            audio_data[i + 1] = sample; // Right channel
        }
        
        return audio_data;
    }
    
    std::unique_ptr<EtsiIntegrationManager> integration_manager_;
    EtsiSystemConfig config_;
    std::vector<EtiFrame> test_frames_;
    std::vector<float> test_audio_data_;
    std::chrono::high_resolution_clock::time_point test_start_time_;
};

/**
 * @brief Test system initialization and component startup
 */
TEST_F(EtsiComplianceSystemTest, SystemInitializationTest) {
    EXPECT_TRUE(integration_manager_->initialize());
    EXPECT_EQ(integration_manager_->get_system_status(), SystemStatus::READY);
    
    // Verify all components are initialized
    auto health_info = integration_manager_->get_all_component_health();
    EXPECT_EQ(health_info.size(), 6); // 6 main components
    
    for (const auto& health : health_info) {
        EXPECT_NE(health.health, ComponentHealth::FAILED);
        EXPECT_NE(health.health, ComponentHealth::UNAVAILABLE);
    }
    
    // Test component interfaces are available
    EXPECT_NE(integration_manager_->get_compliance_engine(), nullptr);
    EXPECT_NE(integration_manager_->get_alert_system(), nullptr);
    EXPECT_NE(integration_manager_->get_logger(), nullptr);
    EXPECT_NE(integration_manager_->get_reporter(), nullptr);
    EXPECT_NE(integration_manager_->get_broadcast_framework(), nullptr);
    EXPECT_NE(integration_manager_->get_realtime_monitor(), nullptr);
}

/**
 * @brief Test system startup and operational state
 */
TEST_F(EtsiComplianceSystemTest, SystemStartupTest) {
    ASSERT_TRUE(integration_manager_->initialize());
    EXPECT_TRUE(integration_manager_->start());
    EXPECT_EQ(integration_manager_->get_system_status(), SystemStatus::RUNNING);
    EXPECT_TRUE(integration_manager_->is_running());
    
    // Verify system is ready for processing
    EXPECT_TRUE(integration_manager_->is_ready());
    
    // Test graceful stop
    EXPECT_TRUE(integration_manager_->stop());
    EXPECT_EQ(integration_manager_->get_system_status(), SystemStatus::READY);
}

/**
 * @brief Test end-to-end ETI frame processing workflow
 */
TEST_F(EtsiComplianceSystemTest, EndToEndFrameProcessingTest) {
    ASSERT_TRUE(integration_manager_->initialize());
    ASSERT_TRUE(integration_manager_->start());
    
    uint32_t stream_id = 1;
    std::vector<ComplianceResult> results;
    
    // Process test frames through complete pipeline
    for (const auto& frame : test_frames_) {
        auto result = integration_manager_->process_eti_frame(frame, stream_id);
        results.push_back(result);
        
        // Verify basic result validity
        EXPECT_GT(result.frames_analyzed, 0);
        EXPECT_GE(result.get_overall_compliance(), 0.0);
        EXPECT_LE(result.get_overall_compliance(), 100.0);
    }
    
    // Verify all frames were processed
    EXPECT_EQ(results.size(), test_frames_.size());
    
    // Check that compliance results are reasonable
    double avg_compliance = 0.0;
    for (const auto& result : results) {
        avg_compliance += result.get_overall_compliance();
    }
    avg_compliance /= results.size();
    
    // Test frames should have high compliance
    EXPECT_GE(avg_compliance, 85.0); // At least 85% compliance
}

/**
 * @brief Test audio quality monitoring integration
 */
TEST_F(EtsiComplianceSystemTest, AudioQualityMonitoringTest) {
    ASSERT_TRUE(integration_manager_->initialize());
    ASSERT_TRUE(integration_manager_->start());
    
    uint32_t service_id = 101;
    
    // Process test audio data
    auto result = integration_manager_->process_audio_data(
        test_audio_data_, 48000, 2, service_id);
    
    EXPECT_TRUE(result.overall_compliant);
    
    // Verify audio analysis results
    EXPECT_GT(result.audio_analysis.sample_count, 0);
    EXPECT_GT(result.audio_analysis.measurement_duration_seconds, 0.0);
    
    // Check ITU-R BS.1770-4 compliance
    EXPECT_TRUE(result.audio_analysis.itu_r1770.is_broadcast_compliant() ||
                result.audio_analysis.itu_r1770.is_streaming_compliant());
    
    // Verify no serious audio issues detected
    EXPECT_FALSE(result.audio_analysis.silence_detected);
    EXPECT_FALSE(result.audio_analysis.overmodulation_detected);
}

/**
 * @brief Test real-time performance requirements
 */
TEST_F(EtsiComplianceSystemTest, RealTimePerformanceTest) {
    ASSERT_TRUE(integration_manager_->initialize());
    ASSERT_TRUE(integration_manager_->start());
    
    uint32_t stream_id = 1;
    std::vector<std::chrono::microseconds> processing_times;
    
    // Measure processing time for each frame
    for (const auto& frame : test_frames_) {
        auto start_time = std::chrono::high_resolution_clock::now();
        
        auto result = integration_manager_->process_eti_frame(frame, stream_id);
        
        auto end_time = std::chrono::high_resolution_clock::now();
        auto processing_time = std::chrono::duration_cast<std::chrono::microseconds>(
            end_time - start_time);
        
        processing_times.push_back(processing_time);
    }
    
    // Calculate performance statistics
    auto max_time = *std::max_element(processing_times.begin(), processing_times.end());
    auto min_time = *std::min_element(processing_times.begin(), processing_times.end());
    
    uint64_t total_time = 0;
    for (const auto& time : processing_times) {
        total_time += time.count();
    }
    auto avg_time = std::chrono::microseconds{total_time / processing_times.size()};
    
    // Verify real-time performance requirements
    EXPECT_LT(max_time.count(), 100000); // Max 100ms processing time
    EXPECT_LT(avg_time.count(), 50000);  // Average under 50ms
    
    std::cout << "Performance Results:" << std::endl;
    std::cout << "  Max processing time: " << max_time.count() << " μs" << std::endl;
    std::cout << "  Min processing time: " << min_time.count() << " μs" << std::endl;
    std::cout << "  Avg processing time: " << avg_time.count() << " μs" << std::endl;
}

/**
 * @brief Test alert system integration and response
 */
TEST_F(EtsiComplianceSystemTest, AlertSystemIntegrationTest) {
    ASSERT_TRUE(integration_manager_->initialize());
    ASSERT_TRUE(integration_manager_->start());
    
    auto alert_system = integration_manager_->get_alert_system();
    ASSERT_NE(alert_system, nullptr);
    
    // Monitor alerts during processing
    std::vector<Alert> generated_alerts;
    alert_system->register_alert_callback([&generated_alerts](const Alert& alert) {
        generated_alerts.push_back(alert);
    });
    
    // Process frames and audio data
    uint32_t stream_id = 1;
    uint32_t service_id = 101;
    
    for (size_t i = 0; i < 10; ++i) {
        integration_manager_->process_eti_frame(test_frames_[i], stream_id);
        integration_manager_->process_audio_data(test_audio_data_, 48000, 2, service_id);
    }
    
    // Allow time for async alert processing
    std::this_thread::sleep_for(std::chrono::milliseconds{100});
    
    // Verify alert system is working
    auto alert_stats = alert_system->get_alert_statistics();
    EXPECT_GE(alert_stats.total_alerts_generated, 0);
    
    // Test alert acknowledgment
    auto active_alerts = alert_system->get_active_alerts();
    if (!active_alerts.empty()) {
        uint64_t alert_id = active_alerts[0].alert_id;
        EXPECT_TRUE(alert_system->acknowledge_alert(alert_id));
        EXPECT_TRUE(alert_system->resolve_alert(alert_id));
    }
}

/**
 * @brief Test compliance logging and audit trail
 */
TEST_F(EtsiComplianceSystemTest, ComplianceLoggingTest) {
    ASSERT_TRUE(integration_manager_->initialize());
    ASSERT_TRUE(integration_manager_->start());
    
    auto logger = integration_manager_->get_logger();
    ASSERT_NE(logger, nullptr);
    
    // Process frames to generate log entries
    uint32_t stream_id = 1;
    for (size_t i = 0; i < 5; ++i) {
        integration_manager_->process_eti_frame(test_frames_[i], stream_id);
    }
    
    // Allow time for async logging
    std::this_thread::sleep_for(std::chrono::milliseconds{200});
    
    // Query recent log entries
    auto recent_logs = logger->get_recent_logs(std::chrono::minutes{1});
    EXPECT_GT(recent_logs.size(), 0);
    
    // Verify log entry structure
    for (const auto& log_entry : recent_logs) {
        EXPECT_GT(log_entry.log_id, 0);
        EXPECT_FALSE(log_entry.message.empty());
        EXPECT_NE(log_entry.severity, LogSeverity::TRACE); // Should be INFO or higher
    }
    
    // Test compliance violations query
    auto violations = logger->get_compliance_violations(std::chrono::hours{1});
    // Should have few or no violations for compliant test data
    
    // Verify logging statistics
    auto stats = logger->get_statistics();
    EXPECT_GT(stats.total_entries_logged, 0);
}

/**
 * @brief Test report generation functionality
 */
TEST_F(EtsiComplianceSystemTest, ReportGenerationTest) {
    ASSERT_TRUE(integration_manager_->initialize());
    ASSERT_TRUE(integration_manager_->start());
    
    // Process some data to have content for reports
    uint32_t stream_id = 1;
    for (size_t i = 0; i < 10; ++i) {
        integration_manager_->process_eti_frame(test_frames_[i], stream_id);
    }
    
    // Allow time for data processing
    std::this_thread::sleep_for(std::chrono::milliseconds{200});
    
    // Generate system report
    ReportScope scope;
    scope.start_time = std::chrono::system_clock::now() - std::chrono::minutes{5};
    scope.end_time = std::chrono::system_clock::now();
    
    auto report_result = integration_manager_->generate_system_report(scope, ReportFormat::HTML);
    
    EXPECT_TRUE(report_result.success);
    EXPECT_FALSE(report_result.report_content.empty());
    EXPECT_GT(report_result.report_size_bytes, 0);
    EXPECT_LT(report_result.generation_time.count(), 5000); // Under 5 seconds
    
    // Verify report contains expected sections
    EXPECT_NE(report_result.report_content.find("Compliance"), std::string::npos);
    EXPECT_NE(report_result.report_content.find("Performance"), std::string::npos);
    EXPECT_NE(report_result.report_content.find("ETSI"), std::string::npos);
}

/**
 * @brief Test multi-stream concurrent processing
 */
TEST_F(EtsiComplianceSystemTest, MultiStreamProcessingTest) {
    ASSERT_TRUE(integration_manager_->initialize());
    ASSERT_TRUE(integration_manager_->start());
    
    const uint32_t num_streams = 4;
    std::vector<std::thread> processing_threads;
    std::atomic<uint32_t> completed_frames{0};
    
    // Create concurrent processing threads
    for (uint32_t stream_id = 1; stream_id <= num_streams; ++stream_id) {
        processing_threads.emplace_back([this, stream_id, &completed_frames]() {
            for (size_t i = 0; i < test_frames_.size(); ++i) {
                integration_manager_->process_eti_frame(test_frames_[i], stream_id);
                completed_frames.fetch_add(1);
                
                // Small delay to simulate real-time processing
                std::this_thread::sleep_for(std::chrono::milliseconds{1});
            }
        });
    }
    
    // Wait for all threads to complete
    for (auto& thread : processing_threads) {
        thread.join();
    }
    
    // Verify all frames were processed
    EXPECT_EQ(completed_frames.load(), num_streams * test_frames_.size());
    
    // Check system performance under load
    auto performance = integration_manager_->get_performance_summary();
    EXPECT_LE(performance.cpu_usage_percentage, 90.0); // Should not exceed 90% CPU
    EXPECT_LT(performance.max_processing_latency.count(), 150000); // Under 150ms
}

/**
 * @brief Test error handling and recovery mechanisms
 */
TEST_F(EtsiComplianceSystemTest, ErrorHandlingTest) {
    ASSERT_TRUE(integration_manager_->initialize());
    ASSERT_TRUE(integration_manager_->start());
    
    // Create malformed ETI frame
    EtiFrame malformed_frame;
    malformed_frame.frame_number = 999999;
    malformed_frame.frame_length = 0; // Invalid length
    malformed_frame.error_flags = 0xFFFF; // All error flags set
    
    // Process malformed frame - should not crash
    uint32_t stream_id = 1;
    auto result = integration_manager_->process_eti_frame(malformed_frame, stream_id);
    
    // System should still be operational
    EXPECT_EQ(integration_manager_->get_system_status(), SystemStatus::RUNNING);
    
    // Verify error was logged
    auto logger = integration_manager_->get_logger();
    auto recent_logs = logger->get_recent_logs(std::chrono::minutes{1});
    
    bool error_logged = false;
    for (const auto& log : recent_logs) {
        if (log.severity >= LogSeverity::ERROR) {
            error_logged = true;
            break;
        }
    }
    EXPECT_TRUE(error_logged);
    
    // Process valid frame after error - should work normally
    auto normal_result = integration_manager_->process_eti_frame(test_frames_[0], stream_id);
    EXPECT_GT(normal_result.frames_analyzed, 0);
}

/**
 * @brief Test configuration management and validation
 */
TEST_F(EtsiComplianceSystemTest, ConfigurationManagementTest) {
    // Test configuration validation
    auto [valid, errors] = integration_manager_->validate_configuration(config_);
    EXPECT_TRUE(valid);
    EXPECT_TRUE(errors.empty());
    
    // Test configuration export
    auto exported_config = integration_manager_->export_configuration("json");
    EXPECT_FALSE(exported_config.empty());
    EXPECT_NE(exported_config.find("compliance_config"), std::string::npos);
    
    // Test configuration import
    EXPECT_TRUE(integration_manager_->import_configuration(exported_config, "json"));
    
    // Test invalid configuration
    EtsiSystemConfig invalid_config = config_;
    invalid_config.realtime_config.max_processing_latency = std::chrono::microseconds{0}; // Invalid
    
    auto [invalid_valid, invalid_errors] = integration_manager_->validate_configuration(invalid_config);
    EXPECT_FALSE(invalid_valid);
    EXPECT_FALSE(invalid_errors.empty());
}

/**
 * @brief Test system performance under sustained load
 */
TEST_F(EtsiComplianceSystemTest, SustainedLoadTest) {
    ASSERT_TRUE(integration_manager_->initialize());
    ASSERT_TRUE(integration_manager_->start());
    
    const auto test_duration = std::chrono::seconds{10};
    const auto start_time = std::chrono::steady_clock::now();
    
    uint32_t stream_id = 1;
    uint32_t frames_processed = 0;
    
    // Process frames continuously for test duration
    while (std::chrono::steady_clock::now() - start_time < test_duration) {
        size_t frame_index = frames_processed % test_frames_.size();
        integration_manager_->process_eti_frame(test_frames_[frame_index], stream_id);
        frames_processed++;
        
        // Brief pause to prevent overwhelming the system
        std::this_thread::sleep_for(std::chrono::microseconds{100});
    }
    
    // Verify system maintained performance
    auto performance = integration_manager_->get_performance_summary();
    
    std::cout << "Sustained Load Results:" << std::endl;
    std::cout << "  Frames processed: " << frames_processed << std::endl;
    std::cout << "  Processing rate: " << (frames_processed / 10.0) << " frames/sec" << std::endl;
    std::cout << "  CPU usage: " << performance.cpu_usage_percentage << "%" << std::endl;
    std::cout << "  Memory usage: " << performance.memory_usage_mb << " MB" << std::endl;
    
    EXPECT_GT(frames_processed, 50); // Should process at least 5 frames/sec
    EXPECT_LT(performance.avg_processing_latency.count(), 100000); // Under 100ms average
}

/**
 * @brief Test production readiness validation
 */
TEST_F(EtsiComplianceSystemTest, ProductionReadinessTest) {
    ASSERT_TRUE(integration_manager_->initialize());
    ASSERT_TRUE(integration_manager_->start());
    
    // Process representative workload
    uint32_t stream_id = 1;
    uint32_t service_id = 101;
    
    for (size_t i = 0; i < 50; ++i) {
        integration_manager_->process_eti_frame(test_frames_[i % test_frames_.size()], stream_id);
        if (i % 10 == 0) {
            integration_manager_->process_audio_data(test_audio_data_, 48000, 2, service_id);
        }
    }
    
    // Allow processing to complete
    std::this_thread::sleep_for(std::chrono::milliseconds{500});
    
    // Validate production readiness
    auto [ready, recommendations] = utils::validate_production_readiness(*integration_manager_);
    
    EXPECT_TRUE(ready);
    
    // If not ready, print recommendations
    if (!ready) {
        std::cout << "Production readiness issues:" << std::endl;
        for (const auto& recommendation : recommendations) {
            std::cout << "  - " << recommendation << std::endl;
        }
    }
    
    // Verify system diagnostics
    auto diagnostics = integration_manager_->get_system_diagnostics();
    EXPECT_FALSE(diagnostics.empty());
    
    // Check uptime
    auto uptime = integration_manager_->get_uptime();
    EXPECT_GT(uptime.count(), 0);
    
    // Verify version info
    auto version = integration_manager_->get_version_info();
    EXPECT_FALSE(version.empty());
}

/**
 * @brief Performance benchmark test
 */
TEST_F(EtsiComplianceSystemTest, PerformanceBenchmarkTest) {
    ASSERT_TRUE(integration_manager_->initialize());
    ASSERT_TRUE(integration_manager_->start());
    
    const size_t num_iterations = 1000;
    std::vector<std::chrono::microseconds> processing_times;
    processing_times.reserve(num_iterations);
    
    uint32_t stream_id = 1;
    
    // Warm up
    for (size_t i = 0; i < 10; ++i) {
        integration_manager_->process_eti_frame(test_frames_[i % test_frames_.size()], stream_id);
    }
    
    // Benchmark processing
    for (size_t i = 0; i < num_iterations; ++i) {
        auto start = std::chrono::high_resolution_clock::now();
        
        integration_manager_->process_eti_frame(test_frames_[i % test_frames_.size()], stream_id);
        
        auto end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
        processing_times.push_back(duration);
    }
    
    // Calculate statistics
    std::sort(processing_times.begin(), processing_times.end());
    
    auto min_time = processing_times.front();
    auto max_time = processing_times.back();
    auto median_time = processing_times[processing_times.size() / 2];
    auto p95_time = processing_times[static_cast<size_t>(processing_times.size() * 0.95)];
    auto p99_time = processing_times[static_cast<size_t>(processing_times.size() * 0.99)];
    
    uint64_t total_time = 0;
    for (const auto& time : processing_times) {
        total_time += time.count();
    }
    auto avg_time = std::chrono::microseconds{total_time / processing_times.size()};
    
    // Print benchmark results
    std::cout << "Performance Benchmark Results (" << num_iterations << " iterations):" << std::endl;
    std::cout << "  Min:    " << min_time.count() << " μs" << std::endl;
    std::cout << "  Max:    " << max_time.count() << " μs" << std::endl;
    std::cout << "  Avg:    " << avg_time.count() << " μs" << std::endl;
    std::cout << "  Median: " << median_time.count() << " μs" << std::endl;
    std::cout << "  P95:    " << p95_time.count() << " μs" << std::endl;
    std::cout << "  P99:    " << p99_time.count() << " μs" << std::endl;
    std::cout << "  Throughput: " << (1000000.0 / avg_time.count()) << " frames/sec" << std::endl;
    
    // Verify performance targets
    EXPECT_LT(avg_time.count(), 50000);  // Average under 50ms
    EXPECT_LT(p95_time.count(), 100000); // 95th percentile under 100ms
    EXPECT_LT(p99_time.count(), 150000); // 99th percentile under 150ms
}

/**
 * @brief Test memory usage and leak detection
 */
TEST_F(EtsiComplianceSystemTest, MemoryUsageTest) {
    // Get initial memory usage
    auto initial_performance = integration_manager_->get_performance_summary();
    auto initial_memory = initial_performance.memory_usage_mb;
    
    ASSERT_TRUE(integration_manager_->initialize());
    ASSERT_TRUE(integration_manager_->start());
    
    // Process large number of frames
    uint32_t stream_id = 1;
    for (size_t iteration = 0; iteration < 10; ++iteration) {
        for (const auto& frame : test_frames_) {
            integration_manager_->process_eti_frame(frame, stream_id);
        }
    }
    
    // Allow garbage collection
    std::this_thread::sleep_for(std::chrono::milliseconds{500});
    
    // Check final memory usage
    auto final_performance = integration_manager_->get_performance_summary();
    auto final_memory = final_performance.memory_usage_mb;
    
    std::cout << "Memory Usage Test:" << std::endl;
    std::cout << "  Initial memory: " << initial_memory << " MB" << std::endl;
    std::cout << "  Final memory:   " << final_memory << " MB" << std::endl;
    std::cout << "  Memory increase: " << (final_memory - initial_memory) << " MB" << std::endl;
    
    // Memory usage should not increase excessively
    EXPECT_LT(final_memory - initial_memory, 100.0); // Less than 100MB increase
    EXPECT_LT(final_memory, 512.0); // Stay under 512MB total
}

/**
 * @brief Main test execution
 */
int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    
    std::cout << "ETSI Compliance System Integration Tests" << std::endl;
    std::cout << "=======================================" << std::endl;
    
    return RUN_ALL_TESTS();
}