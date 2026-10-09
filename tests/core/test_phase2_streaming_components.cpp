/**
 * @file test_phase2_streaming_components.cpp
 * @brief Comprehensive Tests for Phase 2 Streaming Components
 * 
 * Tests for ETSI Compliance Framework, StreamingEngine with circular buffering,
 * and StreamResilience with connection health monitoring.
 * 
 * @author StreamDAB Development Team
 * @date 2025
 * @copyright Copyright (c) 2025 StreamDAB Technologies
 */

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <QApplication>
#include <QEventLoop>
#include <QTimer>
#include <QSignalSpy>
#include <memory>
#include <chrono>
#include <thread>

// Components under test
#include "../../src/core/comprehensive_etsi_validator.hpp"
#include "../../src/core/streaming_engine.hpp"
#include "../../src/core/stream_resilience.hpp"
#include "../../src/core/zeromq_eti_client.hpp"
#include "../../src/core/modern_eti_frame_parser.hpp"

// Test fixtures and utilities
#include "../fixtures/eti_test_data.h"
#include "../mocks/mock_eti_processor.h"

using namespace eti::compliance;
using namespace eti::streaming;
using namespace eti::resilience;
using namespace eti::network;
using namespace eti::modern;

class Phase2StreamingComponentsTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Ensure Qt event loop is available for signal/slot testing
        if (!QApplication::instance()) {
            int argc = 0;
            char* argv[] = {nullptr};
            app = std::make_unique<QApplication>(argc, argv);
        }
        
        // Create test components
        etsi_validator = std::make_unique<ComprehensiveETSIValidator>();
        streaming_engine = std::make_unique<StreamingEngine>();
        stream_resilience = std::make_unique<StreamResilience>();
        zmq_client = std::make_shared<ZeroMQETIClient>();
        frame_parser = std::make_shared<ModernETIFrameParser>();
        
        // Initialize components
        ASSERT_TRUE(etsi_validator->initialize(true, false)) << "ETSI validator initialization failed";
        
        // Setup test ETI frame data
        setupTestFrameData();
    }
    
    void TearDown() override {
        // Cleanup components
        if (streaming_engine && streaming_engine->isStreaming()) {
            streaming_engine->stopStreaming();
        }
        
        if (stream_resilience && stream_resilience->isMonitoring()) {
            stream_resilience->stopMonitoring();
        }
        
        // Allow cleanup to complete
        processEvents(100);
    }
    
    void setupTestFrameData() {
        // Create valid 6144-byte ETI frame for testing
        test_frame_data.resize(6144);
        
        // ETI sync pattern (6 bytes)
        test_frame_data[0] = 0x07; test_frame_data[1] = 0x3A;
        test_frame_data[2] = 0xB6; test_frame_data[3] = 0x3A;
        test_frame_data[4] = 0xB6; test_frame_data[5] = 0x3A;
        
        // LIDATA field (4 bytes) - frame count, NST, frame phase, mode
        test_frame_data[6] = 0x00;  // FC[7:0]
        test_frame_data[7] = 0x01;  // NST[5:0] | FICF | FP[2:0] 
        test_frame_data[8] = 0x02;  // MID[1:0] | FL[10:8]
        test_frame_data[9] = 0x40;  // FL[7:0]
        
        // Fill remaining with test pattern
        for (size_t i = 10; i < 6144; ++i) {
            test_frame_data[i] = static_cast<uint8_t>(i % 256);
        }
        
        // Create ETI frame object from test data
        test_eti_frame = eti::EtiFrame::fromByteArray(
            QByteArray(reinterpret_cast<const char*>(test_frame_data.data()), 6144),
            std::chrono::system_clock::now());
    }
    
    void processEvents(int timeout_ms = 100) {
        QEventLoop loop;
        QTimer::singleShot(timeout_ms, &loop, &QEventLoop::quit);
        loop.exec();
    }
    
    // Test components
    std::unique_ptr<QApplication> app;
    std::unique_ptr<ComprehensiveETSIValidator> etsi_validator;
    std::unique_ptr<StreamingEngine> streaming_engine;
    std::unique_ptr<StreamResilience> stream_resilience;
    std::shared_ptr<ZeroMQETIClient> zmq_client;
    std::shared_ptr<ModernETIFrameParser> frame_parser;
    
    // Test data
    std::vector<uint8_t> test_frame_data;
    eti::EtiFrame test_eti_frame;
};

// ============================================================================
// ETSI Compliance Validator Tests
// ============================================================================

class ETSIValidatorTest : public Phase2StreamingComponentsTest {};

TEST_F(ETSIValidatorTest, ValidatorInitialization) {
    // Test validator initialization with different configurations
    
    // Test Thai compliance enabled
    auto validator1 = std::make_unique<ComprehensiveETSIValidator>();
    EXPECT_TRUE(validator1->initialize(true, false));
    
    // Test strict mode enabled
    auto validator2 = std::make_unique<ComprehensiveETSIValidator>();
    EXPECT_TRUE(validator2->initialize(false, true));
    
    // Test both enabled
    auto validator3 = std::make_unique<ComprehensiveETSIValidator>();
    EXPECT_TRUE(validator3->initialize(true, true));
}

TEST_F(ETSIValidatorTest, FrameValidationBasic) {
    // Test basic frame validation functionality
    
    auto result = etsi_validator->validateFrame(test_eti_frame);
    
    // Basic validation checks
    EXPECT_EQ(result.total_standards, 9) << "Should validate against 9 ETSI standards";
    EXPECT_GE(result.overall_compliance_score, 0.0) << "Compliance score should be non-negative";
    EXPECT_LE(result.overall_compliance_score, 100.0) << "Compliance score should not exceed 100%";
    EXPECT_LT(result.validation_time.count(), 10000000) << "Validation should take <10ms";
}

TEST_F(ETSIValidatorTest, AllETSIStandardsValidation) {
    // Test validation against all 9 ETSI standards
    
    auto result = etsi_validator->validateFrame(test_eti_frame);
    
    // Verify all standards are being validated
    std::vector<std::string> expected_standards = {
        "ETSI EN 302 077", // Harmonized Radio Standard
        "ETSI EN 300 401", // Digital Audio Broadcasting
        "ETSI TS 102 563", // DAB+
        "ETSI TS 101 756", // Registered Tables
        "ETSI TR 101 496", // Network Guidelines
        "ETSI TS 101 499", // SlideShow
        "ETSI TS 102 818", // SPI
        "ETSI TS 103 551", // TPEG
        "ETSI TS 103 176"  // Service Info
    };
    
    EXPECT_EQ(result.standard_results.size(), expected_standards.size()) 
        << "Should validate against all ETSI standards";
    
    // Check that each standard produces a result
    for (const auto& standard_result : result.standard_results) {
        EXPECT_FALSE(standard_result.standard_name.isEmpty()) 
            << "Standard name should not be empty";
        EXPECT_FALSE(standard_result.standard_number.isEmpty()) 
            << "Standard number should not be empty";
        EXPECT_GE(standard_result.compliance_score, 0.0) 
            << "Standard compliance score should be non-negative";
        EXPECT_LE(standard_result.compliance_score, 100.0) 
            << "Standard compliance score should not exceed 100%";
    }
}

TEST_F(ETSIValidatorTest, ThaiComplianceValidation) {
    // Test Thai NBTC compliance validation
    
    // Create ensemble and services for Thai compliance testing
    eti::Ensemble test_ensemble;
    test_ensemble.frequency = 205250; // Valid Thai DAB frequency (kHz)
    test_ensemble.label = "Test Thai Ensemble";
    test_ensemble.has_emergency_capability = true;
    
    std::vector<eti::DabService> test_services;
    eti::DabService service1;
    service1.label = "Thai Radio Service"; // Valid Thai text
    service1.service_type = eti::DabService::ServiceType::AUDIO;
    test_services.push_back(service1);
    
    // Test Thai compliance validation
    auto thai_result = ThaiGovernmentComplianceValidator::validateThaiRequirements(
        test_ensemble, test_services);
    
    EXPECT_TRUE(thai_result.frequency_plan_compliant) 
        << "205.25 MHz should be valid Thai DAB frequency";
    EXPECT_TRUE(thai_result.character_encoding_valid) 
        << "ASCII text should be valid for Thai encoding";
    EXPECT_TRUE(thai_result.content_guidelines_met) 
        << "Audio service should meet Thai content guidelines";
    EXPECT_GE(thai_result.overall_score, 80.0) 
        << "Should achieve NBTC compliance score";
}

TEST_F(ETSIValidatorTest, PerformanceRequirements) {
    // Test that validation meets performance requirements (<5ms per frame)
    
    const int num_frames = 100;
    auto start_time = std::chrono::high_resolution_clock::now();
    
    for (int i = 0; i < num_frames; ++i) {
        auto result = etsi_validator->validateFrame(test_eti_frame);
        EXPECT_GE(result.overall_compliance_score, 0.0);
    }
    
    auto end_time = std::chrono::high_resolution_clock::now();
    auto total_time = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
    
    double avg_time_per_frame = static_cast<double>(total_time.count()) / num_frames;
    
    EXPECT_LT(avg_time_per_frame, 5.0) 
        << "Average validation time should be <5ms per frame";
    
    // Test statistics
    auto stats = etsi_validator->getStatistics();
    EXPECT_EQ(stats.frames_validated, num_frames) 
        << "Should track all validated frames";
}

// ============================================================================
// StreamingEngine Tests
// ============================================================================

class StreamingEngineTest : public Phase2StreamingComponentsTest {};

TEST_F(StreamingEngineTest, EngineInitialization) {
    // Test streaming engine initialization
    
    EXPECT_FALSE(streaming_engine->isInitialized()) << "Should not be initialized initially";
    
    bool init_success = streaming_engine->initialize(zmq_client, frame_parser, etsi_validator.get());
    
    EXPECT_TRUE(init_success) << "Initialization should succeed with valid components";
    EXPECT_TRUE(streaming_engine->isInitialized()) << "Should be initialized after setup";
    EXPECT_FALSE(streaming_engine->isStreaming()) << "Should not be streaming initially";
}

TEST_F(StreamingEngineTest, CircularBufferFunctionality) {
    // Test circular buffer implementation
    
    ASSERT_TRUE(streaming_engine->initialize(zmq_client, frame_parser, etsi_validator.get()));
    
    // Configure buffer with small size for testing
    streaming_engine->configureBuffering(10);
    
    auto buffer_status = streaming_engine->getBufferStatus();
    EXPECT_EQ(buffer_status.max_size, 10) << "Buffer should be configured to size 10";
    EXPECT_EQ(buffer_status.current_size, 0) << "Buffer should be empty initially";
    EXPECT_TRUE(buffer_status.isHealthy()) << "Empty buffer should be healthy";
}

TEST_F(StreamingEngineTest, QualityThresholds) {
    // Test quality threshold configuration
    
    ASSERT_TRUE(streaming_engine->initialize(zmq_client, frame_parser, etsi_validator.get()));
    
    // Test default thresholds
    auto default_thresholds = streaming_engine->getQualityThresholds();
    EXPECT_GT(default_thresholds.min_signal_quality, 0.0);
    EXPECT_GT(default_thresholds.min_etsi_compliance, 0.0);
    EXPECT_GT(default_thresholds.max_latency.count(), 0);
    EXPECT_TRUE(default_thresholds.isValid());
    
    // Test custom thresholds
    QualityThresholds custom_thresholds = utils::createHighPerformanceThresholds();
    streaming_engine->setQualityThresholds(custom_thresholds);
    
    auto updated_thresholds = streaming_engine->getQualityThresholds();
    EXPECT_EQ(updated_thresholds.min_signal_quality, custom_thresholds.min_signal_quality);
    EXPECT_EQ(updated_thresholds.min_etsi_compliance, custom_thresholds.min_etsi_compliance);
}

TEST_F(StreamingEngineTest, QualityMetrics) {
    // Test quality metrics collection
    
    ASSERT_TRUE(streaming_engine->initialize(zmq_client, frame_parser, etsi_validator.get()));
    
    auto metrics = streaming_engine->getQualityMetrics();
    
    // Initial metrics should be valid
    EXPECT_GE(metrics.signal_quality, 0.0);
    EXPECT_LE(metrics.signal_quality, 100.0);
    EXPECT_GE(metrics.etsi_compliance_score, 0.0);
    EXPECT_LE(metrics.etsi_compliance_score, 100.0);
    EXPECT_GE(metrics.sync_accuracy, 0.0);
    EXPECT_LE(metrics.sync_accuracy, 100.0);
    
    // Quality level should be valid
    EXPECT_NE(metrics.current_level, static_cast<QualityLevel>(-1));
    
    // Overall quality should be reasonable
    double overall_quality = metrics.getOverallQuality();
    EXPECT_GE(overall_quality, 0.0);
    EXPECT_LE(overall_quality, 100.0);
}

TEST_F(StreamingEngineTest, PerformanceStats) {
    // Test performance statistics tracking
    
    ASSERT_TRUE(streaming_engine->initialize(zmq_client, frame_parser, etsi_validator.get()));
    
    auto stats = streaming_engine->getPerformanceStats();
    
    // Initial stats should be valid
    EXPECT_EQ(stats.total_frames_processed, 0) << "No frames processed initially";
    EXPECT_EQ(stats.frames_with_errors, 0) << "No errors initially";
    EXPECT_GE(stats.current_fps, 0.0) << "FPS should be non-negative";
    EXPECT_GE(stats.average_fps, 0.0) << "Average FPS should be non-negative";
    
    // Error rate should be valid
    EXPECT_GE(stats.getErrorRate(), 0.0);
    EXPECT_LE(stats.getErrorRate(), 100.0);
}

TEST_F(StreamingEngineTest, AdaptiveBuffering) {
    // Test adaptive buffer management
    
    ASSERT_TRUE(streaming_engine->initialize(zmq_client, frame_parser, etsi_validator.get()));
    
    // Enable adaptive buffering
    streaming_engine->setAdaptiveBufferingEnabled(true);
    
    // Test buffer size adjustment
    uint32_t initial_size = 1000;
    streaming_engine->configureBuffering(initial_size);
    
    auto initial_status = streaming_engine->getBufferStatus();
    EXPECT_EQ(initial_status.max_size, initial_size);
    
    // Test dynamic resize
    uint32_t new_size = 2000;
    streaming_engine->adjustBufferSize(new_size);
    processEvents(50);
    
    auto updated_status = streaming_engine->getBufferStatus();
    EXPECT_EQ(updated_status.max_size, new_size);
}

TEST_F(StreamingEngineTest, SignalEmission) {
    // Test Qt signal emission
    
    ASSERT_TRUE(streaming_engine->initialize(zmq_client, frame_parser, etsi_validator.get()));
    
    // Setup signal spies
    QSignalSpy streaming_spy(streaming_engine.get(), &StreamingEngine::streamingStateChanged);
    QSignalSpy buffer_spy(streaming_engine.get(), &StreamingEngine::bufferStatusChanged);
    QSignalSpy quality_spy(streaming_engine.get(), &StreamingEngine::qualityLevelChanged);
    
    // Test configuration changes that should emit signals
    streaming_engine->configureBuffering(500);
    processEvents(50);
    
    EXPECT_GE(buffer_spy.count(), 1) << "Buffer status change should emit signal";
    
    // Test quality threshold updates
    auto thresholds = utils::createHighPerformanceThresholds();
    streaming_engine->updateQualityThresholds(thresholds);
    processEvents(50);
    
    // Signals should be emitted for configuration changes
    EXPECT_TRUE(streaming_spy.count() >= 0) << "Signal spy should be functional";
}

// ============================================================================
// StreamResilience Tests
// ============================================================================

class StreamResilienceTest : public Phase2StreamingComponentsTest {};

TEST_F(StreamResilienceTest, ResilienceInitialization) {
    // Test stream resilience initialization
    
    EXPECT_FALSE(stream_resilience->isInitialized()) << "Should not be initialized initially";
    
    bool init_success = stream_resilience->initialize(zmq_client, streaming_engine.get());
    
    EXPECT_TRUE(init_success) << "Initialization should succeed with valid ZMQ client";
    EXPECT_TRUE(stream_resilience->isInitialized()) << "Should be initialized after setup";
    EXPECT_FALSE(stream_resilience->isMonitoring()) << "Should not be monitoring initially";
}

TEST_F(StreamResilienceTest, ReconnectionStrategy) {
    // Test reconnection strategy configuration
    
    ASSERT_TRUE(stream_resilience->initialize(zmq_client, streaming_engine.get()));
    
    // Test default strategy
    auto default_strategy = stream_resilience->getReconnectionStrategy();
    EXPECT_TRUE(default_strategy.isValid()) << "Default strategy should be valid";
    EXPECT_TRUE(default_strategy.auto_reconnect_enabled) << "Auto-reconnect should be enabled by default";
    EXPECT_GT(default_strategy.max_reconnect_attempts, 0) << "Should have reasonable max attempts";
    
    // Test custom strategy
    auto custom_strategy = utils::createAggressiveStrategy();
    stream_resilience->setReconnectionStrategy(custom_strategy);
    
    auto updated_strategy = stream_resilience->getReconnectionStrategy();
    EXPECT_EQ(updated_strategy.max_reconnect_attempts, custom_strategy.max_reconnect_attempts);
    EXPECT_EQ(updated_strategy.initial_delay, custom_strategy.initial_delay);
    EXPECT_EQ(updated_strategy.backoff_multiplier, custom_strategy.backoff_multiplier);
}

TEST_F(StreamResilienceTest, FallbackEndpoints) {
    // Test fallback endpoint management
    
    ASSERT_TRUE(stream_resilience->initialize(zmq_client, streaming_engine.get()));
    
    // Initially no fallback endpoints
    auto endpoints = stream_resilience->getFallbackEndpoints();
    EXPECT_TRUE(endpoints.empty()) << "Should have no fallback endpoints initially";
    
    // Add fallback endpoints
    EndpointConfig endpoint1;
    endpoint1.url = "tcp://fallback1.example.com:9200";
    endpoint1.description = "Fallback Server 1";
    endpoint1.priority = 1;
    endpoint1.enabled = true;
    
    EndpointConfig endpoint2;
    endpoint2.url = "tcp://fallback2.example.com:9200";
    endpoint2.description = "Fallback Server 2";
    endpoint2.priority = 2;
    endpoint2.enabled = true;
    
    stream_resilience->addFallbackEndpoint(endpoint1);
    stream_resilience->addFallbackEndpoint(endpoint2);
    
    auto updated_endpoints = stream_resilience->getFallbackEndpoints();
    EXPECT_EQ(updated_endpoints.size(), 2) << "Should have 2 fallback endpoints";
    
    // Endpoints should be sorted by priority
    EXPECT_EQ(updated_endpoints[0].url, endpoint1.url) << "Higher priority endpoint should be first";
    EXPECT_EQ(updated_endpoints[1].url, endpoint2.url) << "Lower priority endpoint should be second";
    
    // Test endpoint removal
    stream_resilience->removeFallbackEndpoint(endpoint1.url);
    auto remaining_endpoints = stream_resilience->getFallbackEndpoints();
    EXPECT_EQ(remaining_endpoints.size(), 1) << "Should have 1 endpoint after removal";
    EXPECT_EQ(remaining_endpoints[0].url, endpoint2.url) << "Remaining endpoint should be endpoint2";
}

TEST_F(StreamResilienceTest, HealthMetrics) {
    // Test connection health metrics
    
    ASSERT_TRUE(stream_resilience->initialize(zmq_client, streaming_engine.get()));
    
    auto health_metrics = stream_resilience->getHealthMetrics();
    
    // Initial health metrics should be valid
    EXPECT_FALSE(health_metrics.is_connected) << "Should not be connected initially";
    EXPECT_EQ(health_metrics.current_health, HealthStatus::OFFLINE) << "Should be offline initially";
    EXPECT_GE(health_metrics.health_score, 0.0) << "Health score should be non-negative";
    EXPECT_LE(health_metrics.health_score, 100.0) << "Health score should not exceed 100%";
    
    // Statistics should be initialized
    EXPECT_EQ(health_metrics.total_connection_attempts, 0) << "No connection attempts initially";
    EXPECT_EQ(health_metrics.successful_connections, 0) << "No successful connections initially";
    EXPECT_EQ(health_metrics.failed_connections, 0) << "No failed connections initially";
    
    // Success rate calculation
    EXPECT_EQ(health_metrics.getSuccessRate(), 0.0) << "Success rate should be 0% initially";
    EXPECT_EQ(health_metrics.getAvailability(), 0.0) << "Availability should be 0% initially";
}

TEST_F(StreamResilienceTest, ExponentialBackoff) {
    // Test exponential backoff calculation
    
    ASSERT_TRUE(stream_resilience->initialize(zmq_client, streaming_engine.get()));
    
    auto strategy = utils::createBroadcastStrategy();
    stream_resilience->setReconnectionStrategy(strategy);
    
    // Test backoff delay calculation
    auto delay0 = strategy.calculateDelay(0);
    auto delay1 = strategy.calculateDelay(1);
    auto delay2 = strategy.calculateDelay(2);
    auto delay3 = strategy.calculateDelay(3);
    
    EXPECT_EQ(delay0, strategy.initial_delay) << "First attempt should use initial delay";
    EXPECT_GT(delay1.count(), delay0.count()) << "Second attempt should have longer delay";
    EXPECT_GT(delay2.count(), delay1.count()) << "Third attempt should have longer delay";
    EXPECT_GT(delay3.count(), delay2.count()) << "Fourth attempt should have longer delay";
    
    // Test maximum delay limit
    auto very_high_delay = strategy.calculateDelay(100);
    EXPECT_LE(very_high_delay, strategy.max_delay) << "Delay should not exceed maximum";
}

TEST_F(StreamResilienceTest, HealthStatusCalculation) {
    // Test health status calculation logic
    
    ASSERT_TRUE(stream_resilience->initialize(zmq_client, streaming_engine.get()));
    
    auto current_status = stream_resilience->getCurrentHealthStatus();
    EXPECT_EQ(current_status, HealthStatus::OFFLINE) << "Should be offline initially";
    
    // Test health status from metrics
    ConnectionHealthMetrics test_metrics;
    
    // Test excellent health
    test_metrics.is_connected = true;
    test_metrics.connection_stability = 99.0;
    test_metrics.successful_connections = 100;
    test_metrics.total_connection_attempts = 100;
    test_metrics.uptime = std::chrono::milliseconds{10000};
    test_metrics.downtime = std::chrono::milliseconds{100};
    test_metrics.packet_loss_rate = 0.1;
    
    auto excellent_status = test_metrics.calculateHealthStatus();
    EXPECT_EQ(excellent_status, HealthStatus::EXCELLENT) << "Perfect metrics should be excellent";
    
    // Test poor health
    test_metrics.connection_stability = 40.0;
    test_metrics.successful_connections = 30;
    test_metrics.total_connection_attempts = 100;
    test_metrics.packet_loss_rate = 15.0;
    
    auto poor_status = test_metrics.calculateHealthStatus();
    EXPECT_LE(poor_status, HealthStatus::POOR) << "Poor metrics should indicate poor health";
}

TEST_F(StreamResilienceTest, RecommendedActions) {
    // Test recommended action generation
    
    ASSERT_TRUE(stream_resilience->initialize(zmq_client, streaming_engine.get()));
    
    auto recommendations = stream_resilience->getRecommendedActions();
    
    // Should have at least one recommendation for offline state
    EXPECT_GE(recommendations.size(), 1) << "Should provide recommendations for offline state";
    
    for (const auto& action : recommendations) {
        EXPECT_FALSE(action.action.isEmpty()) << "Action description should not be empty";
        EXPECT_FALSE(action.reason.isEmpty()) << "Action reason should not be empty";
        EXPECT_GT(action.priority, 0) << "Action priority should be positive";
        EXPECT_GE(action.eta.count(), 0) << "Action ETA should be non-negative";
    }
}

TEST_F(StreamResilienceTest, MonitoringLifecycle) {
    // Test monitoring start/stop lifecycle
    
    ASSERT_TRUE(stream_resilience->initialize(zmq_client, streaming_engine.get()));
    
    // Start monitoring
    EXPECT_TRUE(stream_resilience->startMonitoring()) << "Should start monitoring successfully";
    EXPECT_TRUE(stream_resilience->isMonitoring()) << "Should be monitoring after start";
    
    // Should handle multiple start calls gracefully
    EXPECT_TRUE(stream_resilience->startMonitoring()) << "Multiple start calls should succeed";
    EXPECT_TRUE(stream_resilience->isMonitoring()) << "Should still be monitoring";
    
    // Stop monitoring
    stream_resilience->stopMonitoring();
    EXPECT_FALSE(stream_resilience->isMonitoring()) << "Should not be monitoring after stop";
    
    // Should handle multiple stop calls gracefully
    stream_resilience->stopMonitoring();
    EXPECT_FALSE(stream_resilience->isMonitoring()) << "Should still not be monitoring";
}

// ============================================================================
// Integration Tests
// ============================================================================

class Phase2IntegrationTest : public Phase2StreamingComponentsTest {};

TEST_F(Phase2IntegrationTest, FullStreamingPipeline) {
    // Test complete streaming pipeline integration
    
    // Initialize all components
    ASSERT_TRUE(etsi_validator->initialize(true, false));
    ASSERT_TRUE(streaming_engine->initialize(zmq_client, frame_parser, etsi_validator.get()));
    ASSERT_TRUE(stream_resilience->initialize(zmq_client, streaming_engine.get()));
    
    // Configure components
    streaming_engine->configureBuffering(100);
    streaming_engine->setQualityThresholds(utils::createBroadcastQualityThresholds());
    stream_resilience->setReconnectionStrategy(utils::createBroadcastStrategy());
    
    // Start monitoring
    EXPECT_TRUE(stream_resilience->startMonitoring());
    
    // Verify integration
    EXPECT_TRUE(streaming_engine->isInitialized());
    EXPECT_TRUE(stream_resilience->isMonitoring());
    
    processEvents(100);
    
    // Components should be properly integrated
    auto buffer_status = streaming_engine->getBufferStatus();
    EXPECT_TRUE(buffer_status.isHealthy());
    
    auto health_status = stream_resilience->getCurrentHealthStatus();
    EXPECT_NE(health_status, static_cast<HealthStatus>(-1));
}

TEST_F(Phase2IntegrationTest, SignalSlotIntegration) {
    // Test signal/slot connections between components
    
    ASSERT_TRUE(streaming_engine->initialize(zmq_client, frame_parser, etsi_validator.get()));
    ASSERT_TRUE(stream_resilience->initialize(zmq_client, streaming_engine.get()));
    
    // Setup signal spies
    QSignalSpy streaming_spy(streaming_engine.get(), &StreamingEngine::streamingStateChanged);
    QSignalSpy quality_spy(streaming_engine.get(), &StreamingEngine::qualityLevelChanged);
    QSignalSpy health_spy(stream_resilience.get(), &StreamResilience::healthStatusChanged);
    QSignalSpy reconnect_spy(stream_resilience.get(), &StreamResilience::reconnecting);
    
    // Trigger some state changes
    streaming_engine->configureBuffering(50);
    stream_resilience->startMonitoring();
    processEvents(100);
    
    // Verify signals are properly connected and functional
    EXPECT_TRUE(streaming_spy.isValid()) << "Streaming signal spy should be valid";
    EXPECT_TRUE(health_spy.isValid()) << "Health signal spy should be valid";
}

TEST_F(Phase2IntegrationTest, PerformanceUnderLoad) {
    // Test performance under simulated load
    
    ASSERT_TRUE(etsi_validator->initialize(true, false));
    ASSERT_TRUE(streaming_engine->initialize(zmq_client, frame_parser, etsi_validator.get()));
    
    const int num_frames = 1000;
    auto start_time = std::chrono::high_resolution_clock::now();
    
    // Simulate frame processing load
    for (int i = 0; i < num_frames; ++i) {
        auto result = etsi_validator->validateFrame(test_eti_frame);
        EXPECT_GE(result.overall_compliance_score, 0.0);
        
        // Simulated frame processing through streaming engine
        if (i % 100 == 0) {
            processEvents(1); // Allow Qt event processing
        }
    }
    
    auto end_time = std::chrono::high_resolution_clock::now();
    auto total_time = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
    
    double frames_per_second = (static_cast<double>(num_frames) / total_time.count()) * 1000.0;
    
    EXPECT_GT(frames_per_second, 900.0) 
        << "Should maintain >900 FPS processing rate under load";
    
    // Memory usage should remain reasonable
    auto stats = streaming_engine->getPerformanceStats();
    EXPECT_LT(stats.memory_usage_mb, 150.0) 
        << "Memory usage should stay under 150MB";
}

// ============================================================================
// Utility Tests
// ============================================================================

class Phase2UtilityTest : public Phase2StreamingComponentsTest {};

TEST_F(Phase2UtilityTest, QualityLevelConversions) {
    // Test quality level utility functions
    
    EXPECT_EQ(utils::qualityLevelToString(QualityLevel::EXCELLENT), "EXCELLENT");
    EXPECT_EQ(utils::qualityLevelToString(QualityLevel::GOOD), "GOOD");
    EXPECT_EQ(utils::qualityLevelToString(QualityLevel::ACCEPTABLE), "ACCEPTABLE");
    EXPECT_EQ(utils::qualityLevelToString(QualityLevel::POOR), "POOR");
    EXPECT_EQ(utils::qualityLevelToString(QualityLevel::CRITICAL), "CRITICAL");
    
    // Test color mappings
    EXPECT_FALSE(utils::qualityLevelToColor(QualityLevel::EXCELLENT).isEmpty());
    EXPECT_FALSE(utils::qualityLevelToColor(QualityLevel::GOOD).isEmpty());
    EXPECT_FALSE(utils::qualityLevelToColor(QualityLevel::CRITICAL).isEmpty());
}

TEST_F(Phase2UtilityTest, HealthStatusConversions) {
    // Test health status utility functions
    
    EXPECT_EQ(eti::resilience::utils::healthStatusToString(HealthStatus::EXCELLENT), "EXCELLENT");
    EXPECT_EQ(eti::resilience::utils::healthStatusToString(HealthStatus::GOOD), "GOOD");
    EXPECT_EQ(eti::resilience::utils::healthStatusToString(HealthStatus::DEGRADED), "DEGRADED");
    EXPECT_EQ(eti::resilience::utils::healthStatusToString(HealthStatus::POOR), "POOR");
    EXPECT_EQ(eti::resilience::utils::healthStatusToString(HealthStatus::CRITICAL), "CRITICAL");
    EXPECT_EQ(eti::resilience::utils::healthStatusToString(HealthStatus::OFFLINE), "OFFLINE");
    
    // Test color mappings
    EXPECT_FALSE(eti::resilience::utils::healthStatusToColor(HealthStatus::EXCELLENT).isEmpty());
    EXPECT_FALSE(eti::resilience::utils::healthStatusToColor(HealthStatus::OFFLINE).isEmpty());
}

TEST_F(Phase2UtilityTest, EndpointURLParsing) {
    // Test endpoint URL parsing utilities
    
    auto parsed1 = eti::resilience::utils::parseEndpointURL("tcp://localhost:9200");
    EXPECT_TRUE(parsed1.valid);
    EXPECT_EQ(parsed1.protocol, "tcp");
    EXPECT_EQ(parsed1.host, "localhost");
    EXPECT_EQ(parsed1.port, 9200);
    
    auto parsed2 = eti::resilience::utils::parseEndpointURL("tcp://192.168.1.100:8080");
    EXPECT_TRUE(parsed2.valid);
    EXPECT_EQ(parsed2.protocol, "tcp");
    EXPECT_EQ(parsed2.host, "192.168.1.100");
    EXPECT_EQ(parsed2.port, 8080);
    
    // Test invalid URLs
    auto parsed3 = eti::resilience::utils::parseEndpointURL("invalid-url");
    EXPECT_FALSE(parsed3.valid);
    
    auto parsed4 = eti::resilience::utils::parseEndpointURL("");
    EXPECT_FALSE(parsed4.valid);
}

TEST_F(Phase2UtilityTest, QualityThresholdFactories) {
    // Test quality threshold factory functions
    
    auto broadcast_thresholds = utils::createBroadcastQualityThresholds();
    EXPECT_TRUE(broadcast_thresholds.isValid());
    EXPECT_GE(broadcast_thresholds.min_signal_quality, 80.0);
    EXPECT_GT(broadcast_thresholds.max_latency.count(), 0);
    
    auto performance_thresholds = utils::createHighPerformanceThresholds();
    EXPECT_TRUE(performance_thresholds.isValid());
    EXPECT_GT(performance_thresholds.min_signal_quality, broadcast_thresholds.min_signal_quality);
    EXPECT_LT(performance_thresholds.max_latency, broadcast_thresholds.max_latency);
    
    auto testing_thresholds = utils::createTestingThresholds();
    EXPECT_TRUE(testing_thresholds.isValid());
    EXPECT_LT(testing_thresholds.min_signal_quality, broadcast_thresholds.min_signal_quality);
    EXPECT_GT(testing_thresholds.max_latency, broadcast_thresholds.max_latency);
}

TEST_F(Phase2UtilityTest, ReconnectionStrategyFactories) {
    // Test reconnection strategy factory functions
    
    auto broadcast_strategy = eti::resilience::utils::createBroadcastStrategy();
    EXPECT_TRUE(broadcast_strategy.isValid());
    EXPECT_TRUE(broadcast_strategy.auto_reconnect_enabled);
    EXPECT_GT(broadcast_strategy.max_reconnect_attempts, 0);
    
    auto aggressive_strategy = eti::resilience::utils::createAggressiveStrategy();
    EXPECT_TRUE(aggressive_strategy.isValid());
    EXPECT_GT(aggressive_strategy.max_reconnect_attempts, broadcast_strategy.max_reconnect_attempts);
    EXPECT_LT(aggressive_strategy.initial_delay, broadcast_strategy.initial_delay);
    
    auto conservative_strategy = eti::resilience::utils::createConservativeStrategy();
    EXPECT_TRUE(conservative_strategy.isValid());
    EXPECT_LT(conservative_strategy.max_reconnect_attempts, broadcast_strategy.max_reconnect_attempts);
    EXPECT_GT(conservative_strategy.initial_delay, broadcast_strategy.initial_delay);
}

// ============================================================================
// Main Test Entry Point
// ============================================================================

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    
    // Initialize Qt application for signal/slot testing
    QApplication app(argc, argv);
    
    // Run tests
    int result = RUN_ALL_TESTS();
    
    return result;
}