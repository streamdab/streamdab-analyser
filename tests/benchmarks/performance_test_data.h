#pragma once

#include <vector>
#include <chrono>
#include <memory>
#include <functional>
#include <string>

/**
 * @file performance_test_data.h
 * @brief Performance test datasets for >900 frames/second validation
 * 
 * Provides specialized test data and benchmarking utilities for validating
 * high-performance ETI stream processing capabilities exceeding 900 fps.
 */

namespace PerformanceTestData {

// Forward declarations
class EtsiEtiFrame;
struct PerformanceMetrics;
struct BenchmarkResults;

/**
 * @brief High-performance test data generator optimized for >900 fps testing
 */
class HighPerformanceTestData {
public:
    /**
     * @brief Performance test configuration
     */
    struct PerformanceTestConfig {
        size_t target_fps = 950;              // Target processing rate
        size_t test_duration_seconds = 60;    // Test duration
        size_t warmup_duration_seconds = 5;   // Warmup period
        
        // Data characteristics for performance testing
        enum class TestDataType {
            MINIMAL_PROCESSING,    // Simplest possible frames
            TYPICAL_COMPLEXITY,    // Realistic complexity
            MAXIMUM_COMPLEXITY,    // Most complex processing scenario
            MIXED_COMPLEXITY,      // Varying complexity levels
            STRESS_TEST           // Designed to stress system limits
        } data_type = TestDataType::TYPICAL_COMPLEXITY;
        
        // Performance validation criteria
        double min_sustained_fps = 900.0;     // Minimum acceptable FPS
        std::chrono::microseconds max_latency{1000}; // Maximum processing latency
        size_t max_memory_mb = 512;           // Memory limit
        double max_cpu_percentage = 85.0;     // CPU usage limit
        
        // Test variations
        bool include_jitter_test = true;      // Test timing consistency
        bool include_burst_test = true;       // Test burst processing
        bool include_endurance_test = true;   // Long-duration testing
        bool include_memory_pressure = true;  // Memory stress testing
    };
    
    explicit HighPerformanceTestData(const PerformanceTestConfig& config = PerformanceTestConfig{});
    
    /**
     * @brief Generate test data optimized for >900 fps processing
     * @return High-performance test dataset
     */
    std::vector<EtsiEtiFrame> generate_high_speed_dataset();
    
    /**
     * @brief Generate burst test data (short intense processing bursts)
     * @param burst_count Number of bursts
     * @param frames_per_burst Frames in each burst
     * @return Burst test dataset
     */
    std::vector<EtsiEtiFrame> generate_burst_test_data(
        size_t burst_count = 10,
        size_t frames_per_burst = 1000
    );
    
    /**
     * @brief Generate endurance test data (long-duration processing)
     * @param duration_hours Test duration in hours
     * @return Endurance test dataset
     */
    std::vector<EtsiEtiFrame> generate_endurance_test_data(double duration_hours = 2.0);
    
    /**
     * @brief Generate memory pressure test data
     * @param target_memory_usage_mb Target memory consumption
     * @return Memory-intensive test dataset
     */
    std::vector<EtsiEtiFrame> generate_memory_pressure_data(size_t target_memory_usage_mb = 1024);
    
    /**
     * @brief Generate jitter test data (timing consistency validation)
     * @return Precise timing test dataset
     */
    std::vector<EtsiEtiFrame> generate_jitter_test_data();
    
    /**
     * @brief Generate complete performance test suite
     * @return Comprehensive performance test package
     */
    struct PerformanceTestSuite {
        std::vector<EtsiEtiFrame> speed_test_data;
        std::vector<EtsiEtiFrame> burst_test_data;
        std::vector<EtsiEtiFrame> endurance_test_data;
        std::vector<EtsiEtiFrame> memory_test_data;
        std::vector<EtsiEtiFrame> jitter_test_data;
        
        // Expected performance benchmarks
        struct ExpectedBenchmarks {
            double expected_fps = 950.0;
            std::chrono::microseconds expected_latency{800};
            size_t expected_memory_mb = 256;
            double expected_cpu_usage = 70.0;
            double acceptable_jitter_percentage = 2.0;
        } benchmarks;
    };
    
    PerformanceTestSuite generate_complete_test_suite();
    
    // Configuration management
    void update_config(const PerformanceTestConfig& new_config);
    const PerformanceTestConfig& get_config() const { return config_; }
    
private:
    PerformanceTestConfig config_;
    
    EtsiEtiFrame generate_minimal_frame(uint32_t frame_number);
    EtsiEtiFrame generate_typical_frame(uint32_t frame_number);
    EtsiEtiFrame generate_complex_frame(uint32_t frame_number);
    EtsiEtiFrame generate_stress_frame(uint32_t frame_number);
};

/**
 * @brief Real-time performance benchmark runner
 */
class PerformanceBenchmarkRunner {
public:
    /**
     * @brief Benchmark configuration
     */
    struct BenchmarkConfig {
        std::function<bool(const EtsiEtiFrame&)> processor_callback;
        std::function<void(const PerformanceMetrics&)> metrics_callback;
        
        bool enable_real_time_measurement = true;
        bool enable_memory_monitoring = true;
        bool enable_cpu_monitoring = true;
        bool enable_latency_measurement = true;
        
        // Benchmark parameters
        size_t measurement_interval_frames = 100;
        std::chrono::milliseconds reporting_interval{1000};
        bool stop_on_target_failure = false;
    };
    
    explicit PerformanceBenchmarkRunner(const BenchmarkConfig& config);
    
    /**
     * @brief Run performance benchmark with test data
     * @param test_data Test frames to process
     * @return Benchmark results
     */
    BenchmarkResults run_benchmark(const std::vector<EtsiEtiFrame>& test_data);
    
    /**
     * @brief Run real-time streaming benchmark
     * @param test_data_generator Generator for test frames
     * @param duration_seconds Benchmark duration
     * @return Real-time benchmark results
     */
    BenchmarkResults run_realtime_benchmark(
        std::function<EtsiEtiFrame(uint32_t)> test_data_generator,
        double duration_seconds
    );
    
    /**
     * @brief Validate performance against targets
     * @param results Benchmark results to validate
     * @param targets Target performance criteria
     * @return Validation report
     */
    std::vector<std::string> validate_performance(
        const BenchmarkResults& results,
        const HighPerformanceTestData::PerformanceTestConfig& targets
    );
    
private:
    BenchmarkConfig config_;
    
    void measure_system_resources();
    PerformanceMetrics calculate_metrics(
        const std::chrono::steady_clock::time_point& start,
        const std::chrono::steady_clock::time_point& end,
        size_t frames_processed
    );
};

/**
 * @brief Performance metrics structure
 */
struct PerformanceMetrics {
    // Processing performance
    double frames_per_second = 0.0;
    std::chrono::microseconds avg_processing_time{0};
    std::chrono::microseconds min_processing_time{0};
    std::chrono::microseconds max_processing_time{0};
    
    // System resources
    double cpu_usage_percentage = 0.0;
    size_t memory_usage_bytes = 0;
    double memory_usage_mb = 0.0;
    
    // Timing analysis
    std::chrono::microseconds latency{0};
    double jitter_percentage = 0.0;
    size_t frames_processed = 0;
    size_t frames_dropped = 0;
    
    // Quality metrics
    bool meets_target_fps = false;
    bool within_latency_limits = false;
    bool within_memory_limits = false;
    bool within_cpu_limits = false;
    
    std::chrono::steady_clock::time_point measurement_time;
};

/**
 * @brief Comprehensive benchmark results
 */
struct BenchmarkResults {
    // Overall performance summary
    double overall_fps = 0.0;
    bool meets_performance_targets = false;
    std::string performance_grade; // "EXCELLENT", "GOOD", "ACCEPTABLE", "POOR"
    
    // Detailed metrics over time
    std::vector<PerformanceMetrics> metrics_timeline;
    
    // Statistical analysis
    struct Statistics {
        double mean_fps = 0.0;
        double median_fps = 0.0;
        double min_fps = 0.0;
        double max_fps = 0.0;
        double fps_standard_deviation = 0.0;
        
        std::chrono::microseconds mean_latency{0};
        std::chrono::microseconds median_latency{0};
        std::chrono::microseconds p99_latency{0}; // 99th percentile
        
        double mean_cpu_usage = 0.0;
        double peak_cpu_usage = 0.0;
        double mean_memory_mb = 0.0;
        double peak_memory_mb = 0.0;
    } statistics;
    
    // Performance issues detected
    std::vector<std::string> performance_issues;
    std::vector<std::string> recommendations;
    
    // Test metadata
    std::chrono::steady_clock::time_point test_start_time;
    std::chrono::steady_clock::time_point test_end_time;
    std::chrono::seconds test_duration{0};
    size_t total_frames_tested = 0;
    
    /**
     * @brief Generate comprehensive performance report
     * @return Formatted performance report
     */
    std::vector<std::string> generate_report() const;
    
    /**
     * @brief Check if results meet >900 fps requirement
     * @return True if performance targets are met
     */
    bool meets_900_fps_requirement() const;
};

/**
 * @brief Pre-configured performance test datasets for common scenarios
 */
namespace StandardPerformanceTests {
    
    /**
     * @brief Quick performance validation (30 seconds @ 950 fps)
     * @return Quick test dataset
     */
    std::vector<EtsiEtiFrame> generate_quick_validation_test();
    
    /**
     * @brief Standard performance test (5 minutes @ 950 fps)
     * @return Standard test dataset
     */
    std::vector<EtsiEtiFrame> generate_standard_performance_test();
    
    /**
     * @brief Extended performance test (30 minutes @ 950 fps)
     * @return Extended test dataset
     */
    std::vector<EtsiEtiFrame> generate_extended_performance_test();
    
    /**
     * @brief Stress test dataset (maximum processing complexity)
     * @return Stress test dataset
     */
    std::vector<EtsiEtiFrame> generate_stress_test();
    
    /**
     * @brief Memory limit test (tests memory usage under load)
     * @return Memory stress test dataset
     */
    std::vector<EtsiEtiFrame> generate_memory_limit_test();
    
    /**
     * @brief CPU limit test (tests CPU usage under load)
     * @return CPU stress test dataset
     */
    std::vector<EtsiEtiFrame> generate_cpu_limit_test();
    
    /**
     * @brief Jitter validation test (precise timing requirements)
     * @return Timing precision test dataset
     */
    std::vector<EtsiEtiFrame> generate_jitter_validation_test();
    
    /**
     * @brief Regression test dataset (validates performance consistency)
     * @return Regression test dataset
     */
    std::vector<EtsiEtiFrame> generate_regression_test();
}

/**
 * @brief Performance test utilities
 */
namespace PerformanceTestUtils {
    
    /**
     * @brief Measure system baseline performance
     * @return Baseline performance metrics
     */
    PerformanceMetrics measure_system_baseline();
    
    /**
     * @brief Estimate optimal performance parameters for current system
     * @return Recommended performance configuration
     */
    HighPerformanceTestData::PerformanceTestConfig estimate_optimal_config();
    
    /**
     * @brief Validate system capability for >900 fps processing
     * @return System capability assessment
     */
    struct SystemCapability {
        bool can_achieve_900_fps = false;
        double estimated_max_fps = 0.0;
        std::vector<std::string> limiting_factors;
        std::vector<std::string> optimization_suggestions;
    };
    
    SystemCapability assess_system_capability();
    
    /**
     * @brief Generate performance comparison report
     * @param baseline_results Previous performance results
     * @param current_results Current performance results
     * @return Performance comparison analysis
     */
    std::vector<std::string> compare_performance_results(
        const BenchmarkResults& baseline_results,
        const BenchmarkResults& current_results
    );
    
    /**
     * @brief Optimize test data for maximum performance
     * @param original_data Original test data
     * @return Performance-optimized test data
     */
    std::vector<EtsiEtiFrame> optimize_for_performance(
        const std::vector<EtsiEtiFrame>& original_data
    );
}

/**
 * @brief Automated performance test runner for CI/CD integration
 */
class AutomatedPerformanceValidator {
public:
    struct ValidationConfig {
        bool run_quick_test = true;
        bool run_standard_test = true;
        bool run_stress_test = false;
        bool run_memory_test = true;
        bool run_jitter_test = true;
        
        bool fail_on_target_miss = true;
        bool generate_detailed_reports = true;
        std::string output_directory = "./performance_reports";
        
        // CI/CD integration
        bool save_baseline_results = true;
        bool compare_with_baseline = true;
        double acceptable_performance_regression = 0.05; // 5% regression tolerance
    };
    
    explicit AutomatedPerformanceValidator(const ValidationConfig& config = ValidationConfig{});
    
    /**
     * @brief Run automated performance validation suite
     * @return Overall validation result
     */
    struct ValidationResult {
        bool overall_pass = false;
        std::map<std::string, BenchmarkResults> test_results;
        std::vector<std::string> summary_report;
        std::vector<std::string> failed_tests;
        double overall_performance_score = 0.0; // 0.0 to 1.0
    };
    
    ValidationResult run_validation_suite();
    
    /**
     * @brief Quick validation for development workflow
     * @return Quick validation result
     */
    bool quick_validation();
    
private:
    ValidationConfig config_;
    
    bool save_baseline(const BenchmarkResults& results, const std::string& test_name);
    BenchmarkResults load_baseline(const std::string& test_name);
    bool compare_with_baseline_performance(const BenchmarkResults& current, const BenchmarkResults& baseline);
};

} // namespace PerformanceTestData