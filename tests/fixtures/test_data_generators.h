#pragma once

#include <cstdint>
#include <vector>
#include <string>
#include <random>
#include <chrono>
#include <functional>
#include <memory>

/**
 * @file test_data_generators.h
 * @brief Dynamic ETI test data generators for comprehensive TDD testing
 * 
 * Provides advanced test data generation capabilities including:
 * - Real-time ETI stream simulation
 * - Error injection frameworks
 * - Performance test data generation
 * - Network simulation utilities
 * - ETSI compliance test scenarios
 */

namespace TestDataGenerators {

// Forward declarations
class EtsiEtiFrame;
struct NetworkPacket;
struct StreamConfig;

/**
 * @brief Advanced ETI Frame Generator with comprehensive configuration options
 */
class EtiFrameGenerator {
public:
    /**
     * @brief Configuration for frame generation
     */
    struct GeneratorConfig {
        uint16_t ensemble_id = 0xD001;
        std::string ensemble_label = "Test Ensemble";
        uint8_t service_count = 3;
        std::vector<std::string> service_labels = {"Service 1", "Service 2", "Service 3"};
        bool include_timing_info = true;
        bool include_fic_data = true;
        bool include_msc_data = true;
        uint32_t base_frequency = 220352000; // 220.352 MHz (typical DAB frequency)
        
        // Content generation options
        bool realistic_audio_patterns = true;
        bool include_data_services = false;
        bool include_dabplus_services = true;
        
        // Quality settings
        double audio_quality = 0.95; // 0.0 to 1.0
        uint8_t error_protection_level = 2; // 1-5
        
        // Timing configuration
        std::chrono::microseconds frame_interval{24000}; // 24ms standard
        bool precise_timing = true;
    };
    
    explicit EtiFrameGenerator(const GeneratorConfig& config = GeneratorConfig{});
    
    /**
     * @brief Generate a single ETSI-compliant frame
     * @param frame_number Sequential frame number
     * @return Generated ETI frame
     */
    EtsiEtiFrame generate_frame(uint32_t frame_number);
    
    /**
     * @brief Generate a sequence of frames with realistic timing
     * @param frame_count Number of frames to generate
     * @param start_time Base timestamp for sequence
     * @return Vector of time-sequenced frames
     */
    std::vector<EtsiEtiFrame> generate_sequence(
        size_t frame_count,
        std::chrono::steady_clock::time_point start_time = std::chrono::steady_clock::now()
    );
    
    /**
     * @brief Generate frames with specific service configurations
     * @param service_configs Service-specific configurations
     * @param frame_count Number of frames to generate
     * @return Frames with configured services
     */
    std::vector<EtsiEtiFrame> generate_with_services(
        const std::vector<AudioServiceConfig>& service_configs,
        size_t frame_count
    );
    
    // Configuration management
    void update_config(const GeneratorConfig& new_config);
    const GeneratorConfig& get_config() const { return config_; }
    
    // Statistics
    size_t get_frames_generated() const { return frames_generated_; }
    void reset_statistics() { frames_generated_ = 0; }
    
private:
    GeneratorConfig config_;
    std::mt19937 rng_;
    size_t frames_generated_ = 0;
    
    void generate_fic_data(EtsiEtiFrame& frame, uint32_t frame_number);
    void generate_msc_data(EtsiEtiFrame& frame, uint32_t frame_number);
    void generate_timing_info(EtsiEtiFrame& frame, uint32_t frame_number, std::chrono::steady_clock::time_point base_time);
    
    struct AudioServiceConfig {
        std::string label;
        uint16_t service_id;
        uint8_t sub_channel_id;
        bool is_dab_plus;
        uint16_t bit_rate;
        uint8_t protection_level;
    };
};

/**
 * @brief Error Injection Framework for robustness testing
 */
class ErrorInjectionGenerator {
public:
    /**
     * @brief Types of errors that can be injected
     */
    enum class ErrorType {
        SYNC_CORRUPTION,        // Corrupt sync pattern
        HEADER_CORRUPTION,      // Corrupt LIDATA field
        FIC_CORRUPTION,         // Corrupt FIC data
        MSC_CORRUPTION,         // Corrupt MSC data
        CRC_CORRUPTION,         // Corrupt CRC field
        TIMING_ERROR,           // Invalid timing information
        FRAME_TRUNCATION,       // Incomplete frame
        FRAME_OVERSIZING,       // Oversized frame
        SEQUENCE_DISRUPTION,    // Frame sequence errors
        CONTENT_MISMATCH,       // Content inconsistencies
        NETWORK_CORRUPTION,     // Network-specific errors
        BUFFER_OVERFLOW,        // Buffer-related errors
        RANDOM_CORRUPTION       // Random bit flips
    };
    
    /**
     * @brief Error injection configuration
     */
    struct ErrorConfig {
        std::vector<ErrorType> enabled_errors;
        double error_rate = 0.01; // 1% error rate
        bool cluster_errors = false; // Inject errors in clusters
        size_t cluster_size = 5;
        bool gradual_corruption = false; // Gradually increase error rate
        
        // Error severity
        enum class Severity { MINOR, MODERATE, SEVERE, CRITICAL } severity = Severity::MODERATE;
        
        // Error distribution
        std::vector<double> error_weights; // Custom weights for error types
    };
    
    explicit ErrorInjectionGenerator(const ErrorConfig& config = ErrorConfig{});
    
    /**
     * @brief Inject errors into a clean frame
     * @param clean_frame Original frame
     * @return Corrupted frame with injected errors
     */
    EtsiEtiFrame inject_errors(const EtsiEtiFrame& clean_frame);
    
    /**
     * @brief Inject errors into a frame sequence
     * @param clean_frames Original frame sequence
     * @return Sequence with injected errors
     */
    std::vector<EtsiEtiFrame> inject_sequence_errors(const std::vector<EtsiEtiFrame>& clean_frames);
    
    /**
     * @brief Generate error scenarios for specific testing
     * @param scenario_type Type of error scenario
     * @param base_frames Clean frames to corrupt
     * @return Frames with scenario-specific errors
     */
    std::vector<EtsiEtiFrame> generate_error_scenario(
        ErrorType scenario_type,
        const std::vector<EtsiEtiFrame>& base_frames
    );
    
    // Configuration management
    void update_config(const ErrorConfig& new_config);
    const ErrorConfig& get_config() const { return config_; }
    
    // Statistics
    struct ErrorStatistics {
        size_t total_frames_processed = 0;
        size_t frames_with_errors = 0;
        std::map<ErrorType, size_t> error_counts;
        double actual_error_rate = 0.0;
    };
    
    ErrorStatistics get_statistics() const { return statistics_; }
    void reset_statistics();
    
private:
    ErrorConfig config_;
    std::mt19937 rng_;
    ErrorStatistics statistics_;
    
    void inject_sync_corruption(EtsiEtiFrame& frame);
    void inject_header_corruption(EtsiEtiFrame& frame);
    void inject_fic_corruption(EtsiEtiFrame& frame);
    void inject_msc_corruption(EtsiEtiFrame& frame);
    void inject_crc_corruption(EtsiEtiFrame& frame);
    void inject_timing_error(EtsiEtiFrame& frame);
    void inject_random_corruption(EtsiEtiFrame& frame, size_t bit_count);
    
    bool should_inject_error(ErrorType type);
    ErrorType select_random_error_type();
};

/**
 * @brief Network Stream Simulator for ETI-over-IP testing
 */
class NetworkStreamSimulator {
public:
    /**
     * @brief Network configuration for simulation
     */
    struct NetworkConfig {
        std::string multicast_address = "239.192.0.1";
        uint16_t port = 9200;
        size_t mtu_size = 1500;
        
        // Network characteristics
        double packet_loss_rate = 0.001; // 0.1%
        std::chrono::microseconds latency{1000}; // 1ms
        std::chrono::microseconds jitter{100}; // 100µs
        double bandwidth_mbps = 10.0; // 10 Mbps
        
        // Protocol settings
        bool use_rtp = true;
        bool use_fec = true; // Forward Error Correction
        uint8_t fec_redundancy = 2; // 2 redundant packets
        
        // Timing
        bool simulate_real_time = true;
        std::chrono::microseconds packet_interval{4000}; // 4ms for 250fps
    };
    
    explicit NetworkStreamSimulator(const NetworkConfig& config = NetworkConfig{});
    
    /**
     * @brief Generate network packets from ETI frames
     * @param frames ETI frames to packetize
     * @return Network packets with realistic characteristics
     */
    std::vector<NetworkPacket> generate_network_stream(const std::vector<EtsiEtiFrame>& frames);
    
    /**
     * @brief Simulate network transmission effects
     * @param packets Original packets
     * @return Packets with network effects applied
     */
    std::vector<NetworkPacket> simulate_network_effects(const std::vector<NetworkPacket>& packets);
    
    /**
     * @brief Generate real-time streaming scenario
     * @param frame_generator Source of ETI frames
     * @param duration_seconds Stream duration
     * @param callback Callback for real-time packet delivery
     */
    void simulate_real_time_stream(
        EtiFrameGenerator& frame_generator,
        double duration_seconds,
        std::function<void(const NetworkPacket&)> callback
    );
    
    // Configuration and statistics
    void update_config(const NetworkConfig& new_config);
    const NetworkConfig& get_config() const { return config_; }
    
    struct NetworkStatistics {
        size_t packets_generated = 0;
        size_t packets_lost = 0;
        size_t bytes_transmitted = 0;
        std::chrono::microseconds total_latency{0};
        std::chrono::microseconds max_jitter{0};
        double effective_bandwidth = 0.0;
    };
    
    NetworkStatistics get_statistics() const { return statistics_; }
    void reset_statistics();
    
private:
    NetworkConfig config_;
    std::mt19937 rng_;
    NetworkStatistics statistics_;
    
    NetworkPacket create_rtp_packet(const EtsiEtiFrame& frame, uint32_t sequence_number);
    NetworkPacket create_udp_packet(const EtsiEtiFrame& frame);
    std::vector<NetworkPacket> apply_fec(const std::vector<NetworkPacket>& packets);
    void apply_packet_loss(std::vector<NetworkPacket>& packets);
    void apply_latency_jitter(std::vector<NetworkPacket>& packets);
};

/**
 * @brief Performance Test Data Generator for >900 fps validation
 */
class PerformanceTestGenerator {
public:
    /**
     * @brief Performance test configuration
     */
    struct PerformanceConfig {
        size_t target_fps = 950; // Target frames per second
        size_t test_duration_seconds = 60; // 1 minute test
        size_t warmup_seconds = 5; // Warmup period
        
        // Test characteristics
        bool vary_complexity = true; // Vary frame complexity
        bool include_stress_patterns = true; // Include stress-inducing patterns
        bool measure_memory_usage = true;
        bool measure_cpu_usage = true;
        
        // Data patterns
        enum class DataPattern {
            CONSTANT,      // Constant data pattern
            RANDOM,        // Random data
            WORST_CASE,    // Worst-case processing scenario
            REALISTIC,     // Realistic broadcast content
            MIXED          // Mixed patterns
        } pattern = DataPattern::REALISTIC;
        
        // Performance criteria
        double max_acceptable_jitter = 0.05; // 5% jitter
        std::chrono::microseconds max_processing_time{1000}; // 1ms max per frame
        size_t max_memory_mb = 256; // 256MB memory limit
    };
    
    explicit PerformanceTestGenerator(const PerformanceConfig& config = PerformanceConfig{});
    
    /**
     * @brief Generate high-performance test dataset
     * @return Test frames optimized for performance validation
     */
    std::vector<EtsiEtiFrame> generate_performance_test_data();
    
    /**
     * @brief Generate stress test dataset
     * @return Frames designed to stress system resources
     */
    std::vector<EtsiEtiFrame> generate_stress_test_data();
    
    /**
     * @brief Generate endurance test dataset
     * @param duration_hours Test duration in hours
     * @return Long-duration test dataset
     */
    std::vector<EtsiEtiFrame> generate_endurance_test_data(double duration_hours);
    
    /**
     * @brief Generate memory pressure test data
     * @param target_memory_mb Target memory usage
     * @return Memory-intensive test data
     */
    std::vector<EtsiEtiFrame> generate_memory_test_data(size_t target_memory_mb);
    
    /**
     * @brief Generate real-time performance validation stream
     * @param callback Real-time frame delivery callback
     * @param metrics_callback Performance metrics callback
     */
    void generate_realtime_performance_stream(
        std::function<bool(const EtsiEtiFrame&)> callback,
        std::function<void(const PerformanceMetrics&)> metrics_callback
    );
    
    // Configuration and results
    void update_config(const PerformanceConfig& new_config);
    const PerformanceConfig& get_config() const { return config_; }
    
    struct PerformanceResults {
        double achieved_fps = 0.0;
        bool meets_target = false;
        std::chrono::microseconds avg_processing_time{0};
        std::chrono::microseconds max_processing_time{0};
        size_t peak_memory_usage = 0;
        double cpu_utilization = 0.0;
        double jitter_percentage = 0.0;
        std::vector<std::string> performance_issues;
    };
    
    PerformanceResults get_last_results() const { return last_results_; }
    
private:
    PerformanceConfig config_;
    std::mt19937 rng_;
    PerformanceResults last_results_;
    
    EtsiEtiFrame generate_constant_pattern_frame(uint32_t frame_number);
    EtsiEtiFrame generate_random_pattern_frame(uint32_t frame_number);
    EtsiEtiFrame generate_worst_case_frame(uint32_t frame_number);
    EtsiEtiFrame generate_realistic_frame(uint32_t frame_number);
    EtsiEtiFrame generate_mixed_pattern_frame(uint32_t frame_number);
    
    void measure_performance_metrics(
        const std::chrono::steady_clock::time_point& start,
        const std::chrono::steady_clock::time_point& end,
        size_t frames_processed
    );
    
    struct PerformanceMetrics {
        double current_fps;
        std::chrono::microseconds processing_time;
        size_t memory_usage;
        double cpu_usage;
        std::chrono::steady_clock::time_point timestamp;
    };
};

/**
 * @brief Scenario-based Test Data Generator for integration testing
 */
class ScenarioTestGenerator {
public:
    /**
     * @brief Pre-defined test scenarios
     */
    enum class TestScenario {
        NORMAL_OPERATION,          // Normal broadcast operation
        STARTUP_SEQUENCE,          // System startup scenario
        SHUTDOWN_SEQUENCE,         // System shutdown scenario
        CHANNEL_SWITCHING,         // Channel/service switching
        NETWORK_RECONNECTION,      // Network disconnection/reconnection
        ERROR_RECOVERY,            // Error recovery scenarios
        LOAD_BALANCING,           // Load distribution scenarios
        CAPACITY_LIMITS,          // Maximum capacity testing
        COMPLIANCE_VALIDATION,     // ETSI compliance validation
        REAL_WORLD_SIMULATION,    // Real-world usage patterns
        EDGE_CASES,               // Edge case scenarios
        REGRESSION_TESTING        // Regression test scenarios
    };
    
    /**
     * @brief Generate test data for specific scenario
     * @param scenario Target test scenario
     * @param duration_seconds Scenario duration
     * @return Scenario-specific test data
     */
    std::vector<EtsiEtiFrame> generate_scenario(TestScenario scenario, double duration_seconds);
    
    /**
     * @brief Generate comprehensive test suite
     * @return Complete set of test scenarios
     */
    std::map<TestScenario, std::vector<EtsiEtiFrame>> generate_complete_test_suite();
    
    /**
     * @brief Generate UI/UX testing scenarios
     * @return Test data optimized for GUI testing
     */
    std::vector<EtsiEtiFrame> generate_gui_test_scenarios();
    
    /**
     * @brief Generate Build Manager validation scenarios
     * @return Test data for build system validation
     */
    std::vector<EtsiEtiFrame> generate_build_validation_scenarios();
    
    /**
     * @brief Generate Standards Compliance test scenarios
     * @return ETSI compliance validation test data
     */
    std::vector<EtsiEtiFrame> generate_compliance_test_scenarios();
    
private:
    EtiFrameGenerator frame_generator_;
    ErrorInjectionGenerator error_generator_;
    NetworkStreamSimulator network_simulator_;
    PerformanceTestGenerator performance_generator_;
    
    std::vector<EtsiEtiFrame> generate_normal_operation(double duration);
    std::vector<EtsiEtiFrame> generate_startup_sequence(double duration);
    std::vector<EtsiEtiFrame> generate_error_recovery(double duration);
    std::vector<EtsiEtiFrame> generate_capacity_limits(double duration);
    std::vector<EtsiEtiFrame> generate_edge_cases(double duration);
};

/**
 * @brief Utility functions for test data management
 */
namespace TestDataUtils {
    
    /**
     * @brief Save test data to file for reuse
     * @param frames Test frames to save
     * @param filename Output filename
     * @param format Output format ("binary", "json", "xml")
     * @return Success status
     */
    bool save_test_data(
        const std::vector<EtsiEtiFrame>& frames,
        const std::string& filename,
        const std::string& format = "binary"
    );
    
    /**
     * @brief Load test data from file
     * @param filename Input filename
     * @param format Input format
     * @return Loaded test frames
     */
    std::vector<EtsiEtiFrame> load_test_data(
        const std::string& filename,
        const std::string& format = "binary"
    );
    
    /**
     * @brief Validate test data integrity
     * @param frames Test frames to validate
     * @return Validation report
     */
    std::vector<std::string> validate_test_data(const std::vector<EtsiEtiFrame>& frames);
    
    /**
     * @brief Generate test data summary report
     * @param frames Test frames to analyze
     * @return Summary report
     */
    std::vector<std::string> generate_test_summary(const std::vector<EtsiEtiFrame>& frames);
    
    /**
     * @brief Compare test data sets
     * @param set1 First test data set
     * @param set2 Second test data set
     * @return Comparison report
     */
    std::vector<std::string> compare_test_data(
        const std::vector<EtsiEtiFrame>& set1,
        const std::vector<EtsiEtiFrame>& set2
    );
}

} // namespace TestDataGenerators