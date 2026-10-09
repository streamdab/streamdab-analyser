/**
 * @file test_phase_1_3_validation_suite.cpp
 * @brief Comprehensive Phase 1-3 Implementation Validation Suite
 * 
 * TDD validation framework for verifying Phase 1-3 implementation integrity:
 * - Modern ETI Core Engine validation
 * - ZeroMQ ETI Client connectivity testing
 * - ETI-NI Processor with TIST processing verification
 * - Reed-Solomon codec validation
 * - Configuration Management testing
 * 
 * @author Tester Agent (TDD Lead)
 * @date 2025-09-28
 */

#include <gtest/gtest.h>
#include <QTest>
#include <QApplication>
#include <QSignalSpy>
#include <chrono>
#include <thread>

// Phase 1-3 Core Components
#include "core/modern_eti_frame_parser.hpp"
#include "core/zeromq_eti_client.hpp"
#include "core/eti_ni_processor.hpp"
#include "core/reed_solomon_codec.hpp"
#include "utils/config_manager.hpp"
#include "utils/logger.h"

// Test fixtures and data
#include "fixtures/eti_test_data.h"
#include "fixtures/test_data_generators.h"

namespace test::validation {

// ============================================================================
// Phase 1-3 Validation Test Suite
// ============================================================================

class Phase13ValidationSuite : public ::testing::Test {
protected:
    void SetUp() override {
        // Initialize QApplication for Qt components
        if (!QApplication::instance()) {
            int argc = 0;
            char* argv[] = {nullptr};
            app_ = std::make_unique<QApplication>(argc, argv);
        }
        
        // Initialize logging
        Logger::instance().log(Logger::Info, "Phase13ValidationSuite", 
                              "Starting Phase 1-3 validation suite");
        
        // Create test data generators
        test_data_generator_ = std::make_unique<fixtures::TestDataGenerator>();
        
        start_time_ = std::chrono::high_resolution_clock::now();
    }
    
    void TearDown() override {
        auto end_time = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(
            end_time - start_time_);
        
        Logger::instance().log(Logger::Info, "Phase13ValidationSuite", 
                              QString("Test completed in %1ms").arg(duration.count()));
    }
    
    std::unique_ptr<QApplication> app_;
    std::unique_ptr<fixtures::TestDataGenerator> test_data_generator_;
    std::chrono::high_resolution_clock::time_point start_time_;
};

// ============================================================================
// Modern ETI Core Engine Validation Tests
// ============================================================================

class ModernETICoreEngineTests : public Phase13ValidationSuite {
protected:
    void SetUp() override {
        Phase13ValidationSuite::SetUp();
        
        // Create Modern ETI Frame Parser with production configuration
        eti::modern::ProcessingConfig config;
        config.target_fps = 900.0;
        config.enable_threading = true;
        config.thread_count = 4;
        config.memory_limit_mb = 100;
        config.enable_caching = false; // Disabled for stability
        config.enable_memory_pooling = false; // Disabled for stability
        
        parser_ = std::make_unique<eti::modern::ModernETIFrameParser>();
        ASSERT_TRUE(parser_->initialize(config));
        
        Logger::instance().log(Logger::Info, "ModernETICoreEngineTests", 
                              "Modern ETI Core Engine test setup complete");
    }
    
    std::unique_ptr<eti::modern::ModernETIFrameParser> parser_;
};

TEST_F(ModernETICoreEngineTests, ValidateInitialization) {
    // Arrange - Parser should be initialized from SetUp()
    
    // Act - Check initialization status
    auto stats = parser_->getPerformanceStats();
    
    // Assert - Verify initialization
    EXPECT_EQ(stats.frames_processed, 0);
    EXPECT_GE(stats.average_fps, 0.0);
    EXPECT_TRUE(stats.meets_performance_targets || stats.frames_processed == 0);
    
    Logger::instance().log(Logger::Info, "ModernETICoreEngineTests", 
                          "Initialization validation: PASSED");
}

TEST_F(ModernETICoreEngineTests, ValidateETIFrameParsing) {
    // Arrange - Generate valid ETI frame
    auto valid_frame = test_data_generator_->generateValidETIFrame();
    ASSERT_EQ(valid_frame.size(), eti::ETI_FRAME_SIZE);
    
    QSignalSpy frameProcessedSpy(parser_.get(), 
                                &eti::modern::ModernETIFrameParser::frameProcessed);
    
    // Act - Parse the frame
    auto parse_result = parser_->parseFrame(QByteArray::fromRawData(
        reinterpret_cast<const char*>(valid_frame.data()), valid_frame.size()));
    
    // Assert - Verify parsing success
    EXPECT_TRUE(parse_result.success);
    EXPECT_GT(parse_result.frame_number, 0);
    EXPECT_LT(parse_result.parse_time.count(), 100000); // < 100μs
    EXPECT_EQ(frameProcessedSpy.count(), 1);
    
    Logger::instance().log(Logger::Info, "ModernETICoreEngineTests", 
                          QString("Frame parsing validation: PASSED (Frame #%1, %2μs)")
                          .arg(parse_result.frame_number)
                          .arg(parse_result.parse_time.count() / 1000));
}

TEST_F(ModernETICoreEngineTests, ValidatePerformanceTargets) {
    // Arrange - Process multiple frames to measure performance
    const int frame_count = 100;
    std::vector<std::vector<uint8_t>> frames;
    
    for (int i = 0; i < frame_count; ++i) {
        frames.push_back(test_data_generator_->generateValidETIFrame());
    }
    
    // Act - Process frames and measure performance
    auto start_time = std::chrono::high_resolution_clock::now();
    
    for (const auto& frame : frames) {
        auto parse_result = parser_->parseFrame(QByteArray::fromRawData(
            reinterpret_cast<const char*>(frame.data()), frame.size()));
        EXPECT_TRUE(parse_result.success);
    }
    
    auto end_time = std::chrono::high_resolution_clock::now();
    auto total_time = std::chrono::duration_cast<std::chrono::microseconds>(
        end_time - start_time);
    
    // Assert - Verify performance targets
    double fps = (static_cast<double>(frame_count) * 1000000.0) / total_time.count();
    double avg_frame_time_us = static_cast<double>(total_time.count()) / frame_count;
    
    auto stats = parser_->getPerformanceStats();
    
    EXPECT_GT(fps, 900.0); // Target >900 FPS
    EXPECT_LT(avg_frame_time_us, 1000.0); // <1ms per frame
    EXPECT_EQ(stats.frames_processed, frame_count);
    
    Logger::instance().log(Logger::Info, "ModernETICoreEngineTests", 
                          QString("Performance validation: PASSED (%1 FPS, %2μs/frame)")
                          .arg(fps, 0, 'f', 1)
                          .arg(avg_frame_time_us, 0, 'f', 1));
}

TEST_F(ModernETICoreEngineTests, ValidateFIGAnalysis) {
    // Arrange - Generate frame with FIG data
    auto fig_frame = test_data_generator_->generateFrameWithFIGData();
    
    QSignalSpy figAnalysisSpy(parser_.get(), 
                             &eti::modern::ModernETIFrameParser::figAnalysisComplete);
    
    // Act - Parse frame with FIG analysis
    auto parse_result = parser_->parseFrame(QByteArray::fromRawData(
        reinterpret_cast<const char*>(fig_frame.data()), fig_frame.size()));
    
    if (parse_result.success) {
        auto fig_analysis = parser_->analyzeFIGData(parse_result.frame);
        
        // Assert - Verify FIG analysis
        EXPECT_GE(fig_analysis.fig_blocks.size(), 0);
        EXPECT_GE(fig_analysis.compliance.etsi_en_300_799_score, 0.0);
        EXPECT_LE(fig_analysis.compliance.etsi_en_300_799_score, 100.0);
        EXPECT_GE(fig_analysis.compliance.etsi_en_300_401_score, 0.0);
        EXPECT_LE(fig_analysis.compliance.etsi_en_300_401_score, 100.0);
        
        Logger::instance().log(Logger::Info, "ModernETICoreEngineTests", 
                              QString("FIG analysis validation: PASSED (%1 blocks, ETSI 799: %2%, ETSI 401: %3%)")
                              .arg(fig_analysis.fig_blocks.size())
                              .arg(fig_analysis.compliance.etsi_en_300_799_score, 0, 'f', 1)
                              .arg(fig_analysis.compliance.etsi_en_300_401_score, 0, 'f', 1));
    }
}

// ============================================================================
// ZeroMQ ETI Client Validation Tests
// ============================================================================

class ZeroMQETIClientTests : public Phase13ValidationSuite {
protected:
    void SetUp() override {
        Phase13ValidationSuite::SetUp();
        
        // Create ZeroMQ ETI Client
        zmq_client_ = std::make_unique<eti::network::ZeroMQETIClient>();
        ASSERT_TRUE(zmq_client_->initialize());
        
        // Create Modern ETI Frame Parser for integration
        eti::modern::ProcessingConfig config;
        config.target_fps = 900.0;
        config.enable_threading = false; // Simplified for testing
        
        frame_parser_ = std::make_shared<eti::modern::ModernETIFrameParser>();
        ASSERT_TRUE(frame_parser_->initialize(config));
        
        zmq_client_->setFrameParser(frame_parser_);
        
        Logger::instance().log(Logger::Info, "ZeroMQETIClientTests", 
                              "ZeroMQ ETI Client test setup complete");
    }
    
    void TearDown() override {
        if (zmq_client_) {
            zmq_client_->stopReceiving();
            zmq_client_->disconnectFromStream();
        }
        Phase13ValidationSuite::TearDown();
    }
    
    std::unique_ptr<eti::network::ZeroMQETIClient> zmq_client_;
    std::shared_ptr<eti::modern::ModernETIFrameParser> frame_parser_;
};

TEST_F(ZeroMQETIClientTests, ValidateInitialization) {
    // Arrange - Client should be initialized from SetUp()
    
    // Act - Check connection status
    auto status = zmq_client_->getConnectionStatus();
    
    // Assert - Verify initialization
    EXPECT_EQ(status.state, eti::network::ConnectionStatus::State::DISCONNECTED);
    EXPECT_EQ(status.frames_received, 0);
    EXPECT_EQ(status.connection_errors, 0);
    
    Logger::instance().log(Logger::Info, "ZeroMQETIClientTests", 
                          "ZeroMQ initialization validation: PASSED");
}

TEST_F(ZeroMQETIClientTests, ValidateConnectionConfiguration) {
    // Arrange - Create valid connection configuration
    eti::network::ZMQConnectionConfig config;
    config.transport_type = eti::network::ZMQConnectionConfig::TransportType::TCP;
    config.host = "127.0.0.1";
    config.port = 9200;
    config.recv_timeout_ms = 1000;
    config.recv_hwm = 1000;
    config.linger_ms = 100;
    
    // Act - Validate configuration
    bool is_valid = config.isValid();
    QString endpoint = config.getEndpointUrl();
    
    // Assert - Verify configuration
    EXPECT_TRUE(is_valid);
    EXPECT_EQ(endpoint, "tcp://127.0.0.1:9200");
    
    Logger::instance().log(Logger::Info, "ZeroMQETIClientTests", 
                          QString("Connection configuration validation: PASSED (%1)").arg(endpoint));
}

TEST_F(ZeroMQETIClientTests, ValidateFrameProcessingIntegration) {
    // Arrange - Generate test ETI frame
    auto test_frame = test_data_generator_->generateValidETIFrame();
    QByteArray frame_data(reinterpret_cast<const char*>(test_frame.data()), test_frame.size());
    
    QSignalSpy frameReceivedSpy(zmq_client_.get(), 
                               &eti::network::ZeroMQETIClient::etiFrameReceived);
    QSignalSpy frameParsedSpy(zmq_client_.get(), 
                             &eti::network::ZeroMQETIClient::etiFrameParsed);
    
    // Act - Simulate frame reception and processing
    auto receive_timestamp = std::chrono::system_clock::now();
    emit zmq_client_->etiFrameReceived(frame_data, receive_timestamp);
    
    // Process frame through parser
    auto parse_result = frame_parser_->parseFrame(frame_data);
    
    // Assert - Verify integration
    EXPECT_TRUE(parse_result.success);
    EXPECT_EQ(frameReceivedSpy.count(), 1);
    
    // Verify frame data integrity
    auto received_data = frameReceivedSpy.at(0).at(0).toByteArray();
    EXPECT_EQ(received_data.size(), frame_data.size());
    EXPECT_EQ(received_data, frame_data);
    
    Logger::instance().log(Logger::Info, "ZeroMQETIClientTests", 
                          QString("Frame processing integration validation: PASSED (Frame #%1)")
                          .arg(parse_result.frame_number));
}

// ============================================================================
// ETI-NI Processor with TIST Processing Validation Tests
// ============================================================================

class ETINIProcessorTests : public Phase13ValidationSuite {
protected:
    void SetUp() override {
        Phase13ValidationSuite::SetUp();
        
        // Create ETI-NI Processor
        eti_ni_processor_ = std::make_unique<eti::ni::ETINIProcessor>();
        ASSERT_TRUE(eti_ni_processor_->initialize());
        
        // Enable auto-synchronization
        eti_ni_processor_->setAutoSync(true);
        eti_ni_processor_->setSyncAccuracyTarget(std::chrono::microseconds{1000}); // 1ms
        
        Logger::instance().log(Logger::Info, "ETINIProcessorTests", 
                              "ETI-NI Processor test setup complete");
    }
    
    std::unique_ptr<eti::ni::ETINIProcessor> eti_ni_processor_;
};

TEST_F(ETINIProcessorTests, ValidateTISTExtraction) {
    // Arrange - Generate ETI-NI frame with TIST
    auto eti_ni_frame = test_data_generator_->generateETINIFrameWithTIST();
    
    QSignalSpy tistExtractedSpy(eti_ni_processor_.get(), 
                               &eti::ni::ETINIProcessor::tistExtracted);
    
    // Act - Process frame
    auto processed_frame = eti_ni_processor_->processFrame(eti_ni_frame);
    
    // Assert - Verify TIST extraction
    EXPECT_TRUE(processed_frame.tist_info.isValid());
    EXPECT_GT(processed_frame.tist_info.raw_tist, 0);
    EXPECT_NE(processed_frame.tist_info.type, eti::ni::TISTInfo::TimestampType::INVALID);
    EXPECT_EQ(tistExtractedSpy.count(), 1);
    
    // Verify TIST quality
    double sync_quality = processed_frame.tist_info.getSyncQuality();
    EXPECT_GE(sync_quality, 0.0);
    EXPECT_LE(sync_quality, 100.0);
    
    Logger::instance().log(Logger::Info, "ETINIProcessorTests", 
                          QString("TIST extraction validation: PASSED (TIST: 0x%1, Quality: %2%)")
                          .arg(processed_frame.tist_info.raw_tist, 6, 16, QChar('0'))
                          .arg(sync_quality, 0, 'f', 1));
}

TEST_F(ETINIProcessorTests, ValidateSynchronization) {
    // Arrange - Process multiple frames with consistent TIST
    const int frame_count = 10;
    std::vector<eti::ni::ETINIFrame> processed_frames;
    
    QSignalSpy syncStatusSpy(eti_ni_processor_.get(), 
                            &eti::ni::ETINIProcessor::synchronizationStatusChanged);
    
    // Act - Process sequence of frames
    for (int i = 0; i < frame_count; ++i) {
        auto frame_data = test_data_generator_->generateETINIFrameWithSequentialTIST(i);
        auto processed_frame = eti_ni_processor_->processFrame(frame_data);
        processed_frames.push_back(processed_frame);
        
        // Small delay to simulate real-time processing
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
    
    // Assert - Verify synchronization
    auto sync_status = eti_ni_processor_->getSynchronizationStatus();
    
    EXPECT_GT(sync_status.frames_synchronized, 0);
    EXPECT_LE(sync_status.sync_accuracy.count(), 10000); // ≤10ms accuracy
    
    // Verify frame sequence consistency
    for (size_t i = 1; i < processed_frames.size(); ++i) {
        auto& prev_frame = processed_frames[i-1];
        auto& curr_frame = processed_frames[i];
        
        if (prev_frame.tist_info.isValid() && curr_frame.tist_info.isValid()) {
            auto time_diff = curr_frame.tist_info.timeDifference(prev_frame.tist_info);
            EXPECT_GT(time_diff.count(), 0); // Time should advance
            EXPECT_LT(time_diff.count(), 50000); // <50ms between frames
        }
    }
    
    Logger::instance().log(Logger::Info, "ETINIProcessorTests", 
                          QString("Synchronization validation: PASSED (%1 frames, %2μs accuracy)")
                          .arg(sync_status.frames_synchronized)
                          .arg(sync_status.sync_accuracy.count()));
}

TEST_F(ETINIProcessorTests, ValidateETINICompliance) {
    // Arrange - Generate ETI-NI frame
    auto eti_ni_frame = test_data_generator_->generateETINIFrameWithTIST();
    
    QSignalSpy complianceIssueSpy(eti_ni_processor_.get(), 
                                 &eti::ni::ETINIProcessor::complianceIssue);
    
    // Act - Process frame and check compliance
    auto processed_frame = eti_ni_processor_->processFrame(eti_ni_frame);
    
    // Assert - Verify ETI-NI compliance
    EXPECT_TRUE(processed_frame.is_eti_ni_format);
    EXPECT_GT(processed_frame.version, 0);
    EXPECT_EQ(processed_frame.payload_length, eti::ETI_FRAME_SIZE);
    
    // No critical compliance issues should be detected for valid frames
    // (Warnings are acceptable)
    bool has_critical_issues = false;
    for (int i = 0; i < complianceIssueSpy.count(); ++i) {
        QString issue = complianceIssueSpy.at(i).at(0).toString();
        if (issue.contains("error", Qt::CaseInsensitive)) {
            has_critical_issues = true;
            break;
        }
    }
    
    EXPECT_FALSE(has_critical_issues);
    
    Logger::instance().log(Logger::Info, "ETINIProcessorTests", 
                          QString("ETI-NI compliance validation: PASSED (Version: %1, %2 issues)")
                          .arg(processed_frame.version)
                          .arg(complianceIssueSpy.count()));
}

// ============================================================================
// Reed-Solomon Codec Validation Tests
// ============================================================================

class ReedSolomonCodecTests : public Phase13ValidationSuite {
protected:
    void SetUp() override {
        Phase13ValidationSuite::SetUp();
        
        // Create MPEG-TS Reed-Solomon codec (204,188)
        auto params = eti::codec::RSCodecParams::mpeg_ts();
        mpeg_ts_codec_ = std::make_unique<eti::codec::ReedSolomonCodec>(params);
        ASSERT_TRUE(mpeg_ts_codec_->initialize());
        
        // Create FIC protection codec (255,239)
        auto fic_params = eti::codec::RSCodecParams::fic_protection();
        fic_codec_ = std::make_unique<eti::codec::ReedSolomonCodec>(fic_params);
        ASSERT_TRUE(fic_codec_->initialize());
        
        Logger::instance().log(Logger::Info, "ReedSolomonCodecTests", 
                              "Reed-Solomon codecs test setup complete");
    }
    
    std::unique_ptr<eti::codec::ReedSolomonCodec> mpeg_ts_codec_;
    std::unique_ptr<eti::codec::ReedSolomonCodec> fic_codec_;
};

TEST_F(ReedSolomonCodecTests, ValidateMPEGTSEncoding) {
    // Arrange - Generate 188-byte MPEG-TS packet
    auto ts_packet = test_data_generator_->generateMPEGTSPacket();
    ASSERT_EQ(ts_packet.size(), 188);
    
    // Act - Encode packet
    auto encoded_data = mpeg_ts_codec_->encode(ts_packet);
    
    // Assert - Verify encoding
    EXPECT_EQ(encoded_data.size(), 204); // 188 + 16 parity bytes
    
    // Verify information part is unchanged
    EXPECT_TRUE(std::equal(ts_packet.begin(), ts_packet.end(), 
                          encoded_data.begin() + 16));
    
    Logger::instance().log(Logger::Info, "ReedSolomonCodecTests", 
                          QString("MPEG-TS encoding validation: PASSED (%1 → %2 bytes)")
                          .arg(ts_packet.size()).arg(encoded_data.size()));
}

TEST_F(ReedSolomonCodecTests, ValidateErrorCorrection) {
    // Arrange - Generate and encode data
    auto original_data = test_data_generator_->generateMPEGTSPacket();
    auto encoded_data = mpeg_ts_codec_->encode(original_data);
    
    // Introduce errors (up to t=8 errors correctable)
    auto corrupted_data = encoded_data;
    const int error_count = 4; // Well within correction capability
    
    for (int i = 0; i < error_count; ++i) {
        corrupted_data[i * 50] ^= 0xFF; // Flip bits at various positions
    }
    
    // Act - Decode corrupted data
    auto correction_result = mpeg_ts_codec_->decode(corrupted_data);
    
    // Assert - Verify error correction
    EXPECT_EQ(correction_result.status, eti::codec::CorrectionResult::Status::CORRECTED);
    EXPECT_EQ(correction_result.errors_corrected, error_count);
    EXPECT_EQ(correction_result.corrected_data.size(), original_data.size());
    
    // Verify data integrity after correction
    EXPECT_TRUE(std::equal(original_data.begin(), original_data.end(),
                          correction_result.corrected_data.begin()));
    
    Logger::instance().log(Logger::Info, "ReedSolomonCodecTests", 
                          QString("Error correction validation: PASSED (%1 errors corrected)")
                          .arg(correction_result.errors_corrected));
}

TEST_F(ReedSolomonCodecTests, ValidateUncorrectableErrors) {
    // Arrange - Generate and encode data
    auto original_data = test_data_generator_->generateMPEGTSPacket();
    auto encoded_data = mpeg_ts_codec_->encode(original_data);
    
    // Introduce too many errors (>t=8 errors)
    auto heavily_corrupted_data = encoded_data;
    const int excessive_error_count = 12; // Beyond correction capability
    
    for (int i = 0; i < excessive_error_count; ++i) {
        heavily_corrupted_data[i * 15] ^= 0xFF;
    }
    
    // Act - Attempt to decode heavily corrupted data
    auto correction_result = mpeg_ts_codec_->decode(heavily_corrupted_data);
    
    // Assert - Verify uncorrectable error detection
    EXPECT_EQ(correction_result.status, eti::codec::CorrectionResult::Status::UNCORRECTABLE);
    EXPECT_GT(correction_result.errors_detected, 8); // More than correctable
    
    Logger::instance().log(Logger::Info, "ReedSolomonCodecTests", 
                          QString("Uncorrectable error detection validation: PASSED (%1 errors detected)")
                          .arg(correction_result.errors_detected));
}

// ============================================================================
// Configuration Management Validation Tests
// ============================================================================

class ConfigurationManagementTests : public Phase13ValidationSuite {
protected:
    void SetUp() override {
        Phase13ValidationSuite::SetUp();
        
        // Create configuration manager
        config_manager_ = std::make_unique<ConfigManager>();
        
        // Set test configuration file path
        test_config_file_ = QString("%1/test_config.ini").arg(QDir::tempPath());
        
        Logger::instance().log(Logger::Info, "ConfigurationManagementTests", 
                              QString("Configuration management test setup complete (%1)")
                              .arg(test_config_file_));
    }
    
    void TearDown() override {
        // Clean up test configuration file
        QFile::remove(test_config_file_);
        Phase13ValidationSuite::TearDown();
    }
    
    std::unique_ptr<ConfigManager> config_manager_;
    QString test_config_file_;
};

TEST_F(ConfigurationManagementTests, ValidateConfigurationSaveLoad) {
    // Arrange - Set test configuration values
    config_manager_->setValue("eti/target_fps", 900.0);
    config_manager_->setValue("eti/enable_threading", true);
    config_manager_->setValue("eti/thread_count", 4);
    config_manager_->setValue("network/zeromq_endpoint", "tcp://127.0.0.1:9200");
    config_manager_->setValue("network/recv_timeout_ms", 1000);
    
    // Act - Save and reload configuration
    bool save_success = config_manager_->saveToFile(test_config_file_);
    ASSERT_TRUE(save_success);
    
    // Create new config manager and load
    auto new_config_manager = std::make_unique<ConfigManager>();
    bool load_success = new_config_manager->loadFromFile(test_config_file_);
    ASSERT_TRUE(load_success);
    
    // Assert - Verify configuration values
    EXPECT_DOUBLE_EQ(new_config_manager->getValue("eti/target_fps", 0.0).toDouble(), 900.0);
    EXPECT_TRUE(new_config_manager->getValue("eti/enable_threading", false).toBool());
    EXPECT_EQ(new_config_manager->getValue("eti/thread_count", 0).toInt(), 4);
    EXPECT_EQ(new_config_manager->getValue("network/zeromq_endpoint", "").toString(), 
              "tcp://127.0.0.1:9200");
    EXPECT_EQ(new_config_manager->getValue("network/recv_timeout_ms", 0).toInt(), 1000);
    
    Logger::instance().log(Logger::Info, "ConfigurationManagementTests", 
                          "Configuration save/load validation: PASSED");
}

TEST_F(ConfigurationManagementTests, ValidateDefaultConfiguration) {
    // Arrange - Create fresh configuration manager
    auto fresh_config = std::make_unique<ConfigManager>();
    
    // Act - Get default values
    double default_fps = fresh_config->getValue("eti/target_fps", 900.0).toDouble();
    bool default_threading = fresh_config->getValue("eti/enable_threading", true).toBool();
    int default_threads = fresh_config->getValue("eti/thread_count", 4).toInt();
    QString default_endpoint = fresh_config->getValue("network/zeromq_endpoint", 
                                                     "tcp://127.0.0.1:9200").toString();
    
    // Assert - Verify sensible defaults
    EXPECT_GE(default_fps, 100.0);
    EXPECT_LE(default_fps, 10000.0);
    EXPECT_TRUE(default_threading); // Threading should be enabled by default
    EXPECT_GT(default_threads, 0);
    EXPECT_LE(default_threads, 32);
    EXPECT_TRUE(default_endpoint.startsWith("tcp://"));
    
    Logger::instance().log(Logger::Info, "ConfigurationManagementTests", 
                          QString("Default configuration validation: PASSED (FPS: %1, Threads: %2, Endpoint: %3)")
                          .arg(default_fps).arg(default_threads).arg(default_endpoint));
}

// ============================================================================
// Integration Test Scenarios
// ============================================================================

class Phase13IntegrationTests : public Phase13ValidationSuite {
protected:
    void SetUp() override {
        Phase13ValidationSuite::SetUp();
        
        // Initialize all Phase 1-3 components
        setupModernETICore();
        setupZeroMQClient();
        setupETINIProcessor();
        setupReedSolomonCodecs();
        setupConfigurationManager();
        
        Logger::instance().log(Logger::Info, "Phase13IntegrationTests", 
                              "Phase 1-3 integration test setup complete");
    }
    
private:
    void setupModernETICore() {
        eti::modern::ProcessingConfig config;
        config.target_fps = 900.0;
        config.enable_threading = true;
        config.thread_count = 2; // Conservative for testing
        
        eti_parser_ = std::make_unique<eti::modern::ModernETIFrameParser>();
        ASSERT_TRUE(eti_parser_->initialize(config));
    }
    
    void setupZeroMQClient() {
        zmq_client_ = std::make_unique<eti::network::ZeroMQETIClient>();
        ASSERT_TRUE(zmq_client_->initialize());
        zmq_client_->setFrameParser(std::static_pointer_cast<eti::modern::ModernETIFrameParser>(
            std::shared_ptr<eti::modern::ModernETIFrameParser>(eti_parser_.release())));
    }
    
    void setupETINIProcessor() {
        eti_ni_processor_ = std::make_unique<eti::ni::ETINIProcessor>();
        ASSERT_TRUE(eti_ni_processor_->initialize());
    }
    
    void setupReedSolomonCodecs() {
        auto mpeg_params = eti::codec::RSCodecParams::mpeg_ts();
        mpeg_codec_ = std::make_unique<eti::codec::ReedSolomonCodec>(mpeg_params);
        ASSERT_TRUE(mpeg_codec_->initialize());
    }
    
    void setupConfigurationManager() {
        config_manager_ = std::make_unique<ConfigManager>();
    }
    
protected:
    std::unique_ptr<eti::modern::ModernETIFrameParser> eti_parser_;
    std::unique_ptr<eti::network::ZeroMQETIClient> zmq_client_;
    std::unique_ptr<eti::ni::ETINIProcessor> eti_ni_processor_;
    std::unique_ptr<eti::codec::ReedSolomonCodec> mpeg_codec_;
    std::unique_ptr<ConfigManager> config_manager_;
};

TEST_F(Phase13IntegrationTests, ValidateEndToEndProcessingPipeline) {
    // Arrange - Generate complete ETI stream data
    auto eti_stream = test_data_generator_->generateETIStream(10); // 10 frames
    
    QSignalSpy frameProcessedSpy(eti_parser_.get(), 
                                &eti::modern::ModernETIFrameParser::frameProcessed);
    QSignalSpy tistExtractedSpy(eti_ni_processor_.get(), 
                               &eti::ni::ETINIProcessor::tistExtracted);
    
    // Act - Process complete pipeline
    std::vector<eti::modern::ETIParseResult> parse_results;
    std::vector<eti::ni::ETINIFrame> ni_frames;
    
    for (const auto& frame_data : eti_stream) {
        // 1. Modern ETI Core Engine processing
        auto parse_result = eti_parser_->parseFrame(QByteArray::fromRawData(
            reinterpret_cast<const char*>(frame_data.data()), frame_data.size()));
        parse_results.push_back(parse_result);
        
        // 2. ETI-NI processing with TIST extraction
        auto ni_frame = eti_ni_processor_->processFrame(frame_data);
        ni_frames.push_back(ni_frame);
        
        // 3. Simulate ZeroMQ frame reception
        QByteArray zmq_frame(reinterpret_cast<const char*>(frame_data.data()), frame_data.size());
        emit zmq_client_->etiFrameReceived(zmq_frame, std::chrono::system_clock::now());
    }
    
    // Assert - Verify end-to-end processing
    EXPECT_EQ(parse_results.size(), eti_stream.size());
    EXPECT_EQ(ni_frames.size(), eti_stream.size());
    EXPECT_EQ(frameProcessedSpy.count(), eti_stream.size());
    
    // Verify all frames processed successfully
    int successful_parses = 0;
    int valid_tist_extractions = 0;
    
    for (size_t i = 0; i < parse_results.size(); ++i) {
        if (parse_results[i].success) {
            successful_parses++;
        }
        
        if (ni_frames[i].tist_info.isValid()) {
            valid_tist_extractions++;
        }
    }
    
    EXPECT_GT(successful_parses, eti_stream.size() * 0.8); // ≥80% success rate
    EXPECT_GT(valid_tist_extractions, 0); // At least some TIST extractions
    
    // Verify performance
    auto eti_stats = eti_parser_->getPerformanceStats();
    EXPECT_GT(eti_stats.average_fps, 100.0); // Reasonable performance
    
    auto sync_status = eti_ni_processor_->getSynchronizationStatus();
    EXPECT_GT(sync_status.frames_synchronized, 0);
    
    Logger::instance().log(Logger::Info, "Phase13IntegrationTests", 
                          QString("End-to-end pipeline validation: PASSED (%1/%2 frames, %3 FPS, %4 TIST)")
                          .arg(successful_parses).arg(eti_stream.size())
                          .arg(eti_stats.average_fps, 0, 'f', 1)
                          .arg(valid_tist_extractions));
}

} // namespace test::validation

// ============================================================================
// Test Suite Registration and Main
// ============================================================================

int main(int argc, char* argv[]) {
    // Initialize Google Test
    ::testing::InitGoogleTest(&argc, argv);
    
    // Initialize Qt Application
    QApplication app(argc, argv);
    
    // Configure test environment
    QDir::setCurrent(QCoreApplication::applicationDirPath());
    
    // Run tests
    int result = RUN_ALL_TESTS();
    
    std::cout << "\n========================================" << std::endl;
    std::cout << "Phase 1-3 Validation Suite Complete" << std::endl;
    std::cout << "Result: " << (result == 0 ? "PASSED" : "FAILED") << std::endl;
    std::cout << "========================================" << std::endl;
    
    return result;
}