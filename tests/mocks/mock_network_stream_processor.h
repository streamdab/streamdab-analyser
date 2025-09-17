#pragma once

#include <gmock/gmock.h>
#include <vector>
#include <string>
#include <memory>
#include <chrono>
#include <functional>

/**
 * @file mock_network_stream_processor.h
 * @brief Google Mock objects for network streaming and real-time ETI processing
 * 
 * Comprehensive mocks for testing network stream capture, real-time processing,
 * and performance validation following TDD methodology.
 */

namespace StreamMocks {

// Forward declarations for test interfaces
struct NetworkStreamConfig;
struct StreamStatistics;
struct BufferStatus;
struct PerformanceMetrics;
struct NetworkPacket;
struct EtiFrame;

/**
 * @brief Mock Network Stream Receiver for ETI-over-IP testing
 */
class MockNetworkStreamReceiver {
public:
    virtual ~MockNetworkStreamReceiver() = default;
    
    // Connection management
    MOCK_METHOD(bool, connect, (const std::string& multicast_address, uint16_t port), ());
    MOCK_METHOD(bool, connect_with_config, (const NetworkStreamConfig& config), ());
    MOCK_METHOD(void, disconnect, (), ());
    MOCK_METHOD(bool, is_connected, (), (const));
    MOCK_METHOD(bool, is_receiving, (), (const));
    
    // Stream reception
    MOCK_METHOD(std::vector<uint8_t>, receive_eti_frame, (), ());
    MOCK_METHOD(std::vector<NetworkPacket>, receive_packet_batch, (size_t max_packets), ());
    MOCK_METHOD(bool, has_pending_data, (), (const));
    MOCK_METHOD(size_t, get_available_frame_count, (), (const));
    
    // Buffer management
    MOCK_METHOD(void, set_buffer_size, (size_t buffer_frames), ());
    MOCK_METHOD(BufferStatus, get_buffer_status, (), (const));
    MOCK_METHOD(void, flush_buffers, (), ());
    MOCK_METHOD(bool, is_buffer_overflow, (), (const));
    MOCK_METHOD(bool, is_buffer_underflow, (), (const));
    
    // Performance monitoring
    MOCK_METHOD(StreamStatistics, get_stream_statistics, (), (const));
    MOCK_METHOD(PerformanceMetrics, get_performance_metrics, (), (const));
    MOCK_METHOD(double, get_frame_rate, (), (const));
    MOCK_METHOD(size_t, get_frames_received, (), (const));
    MOCK_METHOD(size_t, get_frames_dropped, (), (const));
    
    // Error handling
    MOCK_METHOD(std::vector<std::string>, get_error_messages, (), (const));
    MOCK_METHOD(void, clear_errors, (), ());
    MOCK_METHOD(bool, has_errors, (), (const));
    MOCK_METHOD(std::string, get_last_error, (), (const));
    
    // Callback registration
    MOCK_METHOD(void, set_frame_callback, (std::function<void(const EtiFrame&)> callback), ());
    MOCK_METHOD(void, set_error_callback, (std::function<void(const std::string&)> callback), ());
    MOCK_METHOD(void, set_statistics_callback, (std::function<void(const StreamStatistics&)> callback), ());
};

/**
 * @brief Mock Real-time ETI Stream Processor for >900 frames/second testing
 */
class MockRealTimeEtiProcessor {
public:
    virtual ~MockRealTimeEtiProcessor() = default;
    
    // Real-time processing
    MOCK_METHOD(bool, start_processing, (), ());
    MOCK_METHOD(void, stop_processing, (), ());
    MOCK_METHOD(bool, is_processing, (), (const));
    MOCK_METHOD(bool, process_frame_realtime, (const EtiFrame& frame), ());
    MOCK_METHOD(bool, process_frame_batch, (const std::vector<EtiFrame>& frames), ());
    
    // Performance validation
    MOCK_METHOD(double, get_processing_rate, (), (const)); // frames per second
    MOCK_METHOD(bool, meets_performance_target, (double target_fps), (const));
    MOCK_METHOD(std::chrono::microseconds, get_average_processing_time, (), (const));
    MOCK_METHOD(std::chrono::microseconds, get_max_processing_time, (), (const));
    MOCK_METHOD(PerformanceMetrics, get_detailed_metrics, (), (const));
    
    // Queue management
    MOCK_METHOD(void, set_queue_size, (size_t max_frames), ());
    MOCK_METHOD(size_t, get_queue_length, (), (const));
    MOCK_METHOD(bool, enqueue_frame, (const EtiFrame& frame), ());
    MOCK_METHOD(bool, dequeue_frame, (EtiFrame& frame), ());
    MOCK_METHOD(void, clear_queue, (), ());
    
    // Threading control
    MOCK_METHOD(void, set_thread_count, (size_t thread_count), ());
    MOCK_METHOD(size_t, get_active_thread_count, (), (const));
    MOCK_METHOD(void, set_thread_priority, (int priority), ());
    
    // Memory management
    MOCK_METHOD(size_t, get_memory_usage, (), (const));
    MOCK_METHOD(void, optimize_memory_usage, (), ());
    MOCK_METHOD(bool, is_memory_limit_exceeded, (), (const));
};

/**
 * @brief Mock Buffer Manager for stream buffering and timing
 */
class MockStreamBufferManager {
public:
    virtual ~MockStreamBufferManager() = default;
    
    // Buffer operations
    MOCK_METHOD(bool, add_frame, (const EtiFrame& frame), ());
    MOCK_METHOD(bool, get_frame, (EtiFrame& frame), ());
    MOCK_METHOD(bool, peek_frame, (EtiFrame& frame), (const));
    MOCK_METHOD(void, clear_buffer, (), ());
    
    // Buffer status
    MOCK_METHOD(size_t, get_buffer_size, (), (const));
    MOCK_METHOD(size_t, get_used_capacity, (), (const));
    MOCK_METHOD(size_t, get_free_capacity, (), (const));
    MOCK_METHOD(double, get_utilization_percentage, (), (const));
    
    // Timing management
    MOCK_METHOD(void, set_target_latency, (std::chrono::milliseconds latency), ());
    MOCK_METHOD(std::chrono::milliseconds, get_current_latency, (), (const));
    MOCK_METHOD(bool, is_latency_acceptable, (), (const));
    
    // Flow control
    MOCK_METHOD(void, pause_buffering, (), ());
    MOCK_METHOD(void, resume_buffering, (), ());
    MOCK_METHOD(bool, is_buffering_paused, (), (const));
    
    // Statistics
    MOCK_METHOD(BufferStatus, get_buffer_statistics, (), (const));
    MOCK_METHOD(size_t, get_frames_buffered, (), (const));
    MOCK_METHOD(size_t, get_frames_dropped, (), (const));
};

/**
 * @brief Mock Performance Monitor for real-time metrics
 */
class MockPerformanceMonitor {
public:
    virtual ~MockPerformanceMonitor() = default;
    
    // Performance tracking
    MOCK_METHOD(void, start_monitoring, (), ());
    MOCK_METHOD(void, stop_monitoring, (), ());
    MOCK_METHOD(bool, is_monitoring, (), (const));
    
    // Frame processing metrics
    MOCK_METHOD(void, record_frame_processed, (std::chrono::microseconds processing_time), ());
    MOCK_METHOD(void, record_frame_dropped, (), ());
    MOCK_METHOD(void, record_frame_error, (const std::string& error), ());
    
    // Real-time metrics
    MOCK_METHOD(double, get_current_fps, (), (const));
    MOCK_METHOD(double, get_average_fps, (), (const));
    MOCK_METHOD(double, get_peak_fps, (), (const));
    MOCK_METHOD(bool, is_meeting_target, (double target_fps), (const));
    
    // Latency metrics
    MOCK_METHOD(std::chrono::microseconds, get_current_latency, (), (const));
    MOCK_METHOD(std::chrono::microseconds, get_average_latency, (), (const));
    MOCK_METHOD(std::chrono::microseconds, get_max_latency, (), (const));
    
    // Resource usage
    MOCK_METHOD(double, get_cpu_usage, (), (const));
    MOCK_METHOD(size_t, get_memory_usage, (), (const));
    MOCK_METHOD(double, get_network_bandwidth, (), (const));
    
    // Comprehensive metrics
    MOCK_METHOD(PerformanceMetrics, get_comprehensive_metrics, (), (const));
    MOCK_METHOD(void, reset_metrics, (), ());
    MOCK_METHOD(std::vector<std::string>, generate_performance_report, (), (const));
};

/**
 * @brief Mock Audio Stream Decoder for DAB/DAB+ testing
 */
class MockAudioStreamDecoder {
public:
    virtual ~MockAudioStreamDecoder() = default;
    
    // Stream decoding
    MOCK_METHOD(bool, decode_audio_stream, (const std::vector<uint8_t>& stream_data), ());
    MOCK_METHOD(bool, decode_dab_audio, (const std::vector<uint8_t>& dab_data), ());
    MOCK_METHOD(bool, decode_dabplus_audio, (const std::vector<uint8_t>& aac_data), ());
    
    // Format detection
    MOCK_METHOD(std::string, detect_audio_format, (const std::vector<uint8_t>& data), (const));
    MOCK_METHOD(bool, is_dab_format, (const std::vector<uint8_t>& data), (const));
    MOCK_METHOD(bool, is_dabplus_format, (const std::vector<uint8_t>& data), (const));
    
    // Audio properties
    MOCK_METHOD(int, get_sample_rate, (), (const));
    MOCK_METHOD(int, get_bit_rate, (), (const));
    MOCK_METHOD(int, get_channel_count, (), (const));
    MOCK_METHOD(std::string, get_audio_format, (), (const));
    
    // Decoded output
    MOCK_METHOD(std::vector<int16_t>, get_audio_samples, (), (const));
    MOCK_METHOD(size_t, get_available_sample_count, (), (const));
    MOCK_METHOD(bool, has_audio_output, (), (const));
    
    // Quality metrics
    MOCK_METHOD(double, get_signal_quality, (), (const));
    MOCK_METHOD(bool, has_decoding_errors, (), (const));
    MOCK_METHOD(std::vector<std::string>, get_decoding_errors, (), (const));
};

/**
 * @brief Mock FIG Processor for Fast Information Group analysis
 */
class MockFigProcessor {
public:
    virtual ~MockFigProcessor() = default;
    
    // FIG processing
    MOCK_METHOD(bool, process_fig_block, (const std::vector<uint8_t>& fic_data), ());
    MOCK_METHOD(bool, process_fig_type, (uint8_t fig_type, const std::vector<uint8_t>& fig_data), ());
    
    // Ensemble information (FIG 0/0)
    MOCK_METHOD(bool, extract_ensemble_info, (uint16_t& ensemble_id, std::string& label), ());
    MOCK_METHOD(uint16_t, get_ensemble_id, (), (const));
    MOCK_METHOD(std::string, get_ensemble_label, (), (const));
    
    // Service organization (FIG 0/1, 0/2)
    MOCK_METHOD(std::vector<uint8_t>, get_sub_channel_ids, (), (const));
    MOCK_METHOD(std::vector<uint16_t>, get_service_ids, (), (const));
    MOCK_METHOD(bool, get_service_info, (uint16_t service_id, std::string& label, uint8_t& sub_channel), ());
    
    // Service labels (FIG 1/0, 1/1)
    MOCK_METHOD(std::map<uint16_t, std::string>, get_service_labels, (), (const));
    MOCK_METHOD(std::string, get_service_label, (uint16_t service_id), (const));
    
    // Configuration validation
    MOCK_METHOD(bool, validate_ensemble_configuration, (), (const));
    MOCK_METHOD(std::vector<std::string>, get_configuration_errors, (), (const));
    MOCK_METHOD(bool, is_configuration_complete, (), (const));
};

/**
 * @brief Mock ETI Stream Validator for ETSI compliance testing
 */
class MockEtiStreamValidator {
public:
    virtual ~MockEtiStreamValidator() = default;
    
    // Frame validation
    MOCK_METHOD(bool, validate_frame, (const EtiFrame& frame), ());
    MOCK_METHOD(bool, validate_frame_sequence, (const std::vector<EtiFrame>& frames), ());
    MOCK_METHOD(bool, validate_sync_pattern, (const EtiFrame& frame), ());
    MOCK_METHOD(bool, validate_frame_structure, (const EtiFrame& frame), ());
    MOCK_METHOD(bool, validate_crc, (const EtiFrame& frame), ());
    
    // ETSI compliance
    MOCK_METHOD(bool, is_etsi_compliant, (const EtiFrame& frame), ());
    MOCK_METHOD(std::vector<std::string>, get_compliance_violations, (const EtiFrame& frame), ());
    MOCK_METHOD(double, calculate_compliance_score, (const std::vector<EtiFrame>& frames), ());
    
    // Timing validation
    MOCK_METHOD(bool, validate_frame_timing, (const std::vector<EtiFrame>& frames), ());
    MOCK_METHOD(bool, validate_tist_sequence, (const std::vector<EtiFrame>& frames), ());
    MOCK_METHOD(std::chrono::microseconds, calculate_timing_drift, (const std::vector<EtiFrame>& frames), ());
    
    // Stream analysis
    MOCK_METHOD(StreamStatistics, analyze_stream, (const std::vector<EtiFrame>& frames), ());
    MOCK_METHOD(std::vector<std::string>, generate_validation_report, (const std::vector<EtiFrame>& frames), ());
};

/**
 * @brief Test data structures for mock integration
 */
struct NetworkStreamConfig {
    std::string multicast_address = "239.192.0.1";
    uint16_t port = 9200;
    size_t buffer_size = 1000;
    std::chrono::milliseconds timeout{5000};
    bool enable_flow_control = true;
};

struct StreamStatistics {
    size_t frames_received = 0;
    size_t frames_processed = 0;
    size_t frames_dropped = 0;
    size_t errors_detected = 0;
    double average_frame_rate = 0.0;
    std::chrono::microseconds average_latency{0};
    std::chrono::steady_clock::time_point start_time;
    std::chrono::steady_clock::time_point last_update;
};

struct BufferStatus {
    size_t capacity = 0;
    size_t used = 0;
    size_t free = 0;
    double utilization = 0.0;
    bool overflow = false;
    bool underflow = false;
    std::chrono::milliseconds latency{0};
};

struct PerformanceMetrics {
    double current_fps = 0.0;
    double average_fps = 0.0;
    double peak_fps = 0.0;
    std::chrono::microseconds min_processing_time{0};
    std::chrono::microseconds avg_processing_time{0};
    std::chrono::microseconds max_processing_time{0};
    double cpu_usage = 0.0;
    size_t memory_usage = 0;
    double network_bandwidth = 0.0;
    bool meets_target = false;
};

struct NetworkPacket {
    std::vector<uint8_t> data;
    std::chrono::steady_clock::time_point timestamp;
    size_t sequence_number = 0;
    bool is_valid = true;
};

/**
 * @brief Factory for creating pre-configured mock objects
 */
class NetworkStreamMockFactory {
public:
    /**
     * @brief Create mock for successful high-performance stream processing
     * Configured to simulate >900 frames/second processing capability
     */
    static std::unique_ptr<MockRealTimeEtiProcessor> create_high_performance_processor();
    
    /**
     * @brief Create mock for network stream with realistic characteristics
     * Simulates typical ETI-over-IP multicast stream
     */
    static std::unique_ptr<MockNetworkStreamReceiver> create_realistic_network_receiver();
    
    /**
     * @brief Create mock for stress testing scenarios
     * Simulates high load and error conditions
     */
    static std::unique_ptr<MockPerformanceMonitor> create_stress_test_monitor();
    
    /**
     * @brief Create complete mock pipeline for integration testing
     * Returns configured mocks for end-to-end stream processing
     */
    static std::tuple<
        std::unique_ptr<MockNetworkStreamReceiver>,
        std::unique_ptr<MockRealTimeEtiProcessor>,
        std::unique_ptr<MockStreamBufferManager>,
        std::unique_ptr<MockPerformanceMonitor>
    > create_complete_stream_pipeline();
    
    /**
     * @brief Create mocks for UI/UX Agent testing
     * Configured for GUI component integration
     */
    static std::tuple<
        std::unique_ptr<MockFigProcessor>,
        std::unique_ptr<MockAudioStreamDecoder>,
        std::unique_ptr<MockEtiStreamValidator>
    > create_gui_testing_mocks();
    
    /**
     * @brief Create mocks for Build Manager validation
     * Configured for build system integration testing
     */
    static std::unique_ptr<MockEtiStreamValidator> create_build_validation_mock();
    
    /**
     * @brief Create mocks for Standards Compliance testing
     * Configured for ETSI compliance validation
     */
    static std::unique_ptr<MockEtiStreamValidator> create_compliance_testing_mock();
};

} // namespace StreamMocks