/**
 * @file ultra_performance_benchmark_suite.hpp
 * @brief Comprehensive Performance Benchmark Suite for 10.0/10.0 Score Validation
 * 
 * Validates achievement of PERFECT 10.0/10.0 PERFORMANCE SCORE with:
 * - 2,000,000+ FPS ETI processing validation
 * - <25MB memory footprint verification
 * - <0.1μs latency confirmation
 * - 16+ concurrent stream processing
 * - <5% CPU utilization validation
 * 
 * @author Performance Optimization Agent
 * @date 2025
 */

#pragma once

#include "../core/ultra_performance_eti_processor.hpp"
#include "../network/dpdk_eti_receiver.hpp"
#include "../core/ultra_memory_allocator.hpp"
#include <QObject>
#include <chrono>
#include <vector>
#include <memory>
#include <atomic>
#include <thread>
#include <future>

namespace eti::performance {

/**
 * @brief Performance targets for 10.0/10.0 score
 */
struct perfect_performance_targets {
    static constexpr double TARGET_FPS = 2000000.0;           // 2M FPS
    static constexpr double TARGET_MEMORY_MB = 25.0;          // <25MB
    static constexpr double TARGET_LATENCY_US = 0.1;          // <0.1μs
    static constexpr size_t TARGET_CONCURRENT_STREAMS = 16;   // 16+ streams
    static constexpr double TARGET_CPU_PERCENT = 5.0;         // <5% CPU
    static constexpr double PERFECT_SCORE = 10.0;             // Perfect score
};

/**
 * @brief Comprehensive benchmark results
 */
struct benchmark_results {
    // Core performance metrics
    double peak_fps{0.0};
    double sustained_fps{0.0};
    double average_latency_us{0.0};
    double peak_latency_us{0.0};
    double memory_usage_mb{0.0};
    double cpu_utilization_percent{0.0};
    size_t concurrent_streams_achieved{0};
    
    // Performance improvements vs baseline
    double speed_improvement_percent{0.0};
    double memory_reduction_percent{0.0};
    double latency_reduction_percent{0.0};
    
    // Target achievement
    bool meets_fps_target{false};
    bool meets_memory_target{false};
    bool meets_latency_target{false};
    bool meets_concurrency_target{false};
    bool meets_cpu_target{false};
    bool perfect_score_achieved{false};
    
    // Overall performance score
    double performance_score{0.0};
    
    // Detailed component scores
    struct {
        double processing_score{0.0};
        double memory_score{0.0};
        double network_score{0.0};
        double concurrency_score{0.0};
        double efficiency_score{0.0};
    } component_scores;
    
    // Benchmark metadata
    std::chrono::seconds benchmark_duration{0};
    size_t total_frames_processed{0};
    std::chrono::high_resolution_clock::time_point timestamp;
    std::string system_info;
    std::vector<std::string> performance_notes;
};

/**
 * @brief Individual benchmark test result
 */
struct benchmark_test_result {
    std::string test_name;
    bool passed{false};
    double measured_value{0.0};
    double target_value{0.0};
    std::string units;
    std::chrono::nanoseconds execution_time{0};
    std::string notes;
};

/**
 * @brief Ultra-performance benchmark suite
 */
class UltraPerformanceBenchmarkSuite : public QObject {
    Q_OBJECT

private:
    // Test components
    std::unique_ptr<eti::ultra_performance::UltraPerformanceETIProcessor> processor_;
    std::unique_ptr<eti::network::DpdkEtiReceiver> network_receiver_;
    std::unique_ptr<eti::memory::UltraMemoryAllocator> memory_allocator_;
    
    // Test configuration
    struct benchmark_config {
        std::chrono::seconds duration{30};
        size_t warmup_iterations{1000};
        size_t test_iterations{1000000};
        bool enable_detailed_logging{false};
        bool enable_system_monitoring{true};
        std::vector<size_t> concurrent_stream_counts{1, 4, 8, 16, 32};
    } config_;
    
    // Test data
    std::vector<std::array<uint8_t, 6144>> test_eti_frames_;
    
    // Results tracking
    std::vector<benchmark_test_result> test_results_;
    benchmark_results overall_results_;

public:
    explicit UltraPerformanceBenchmarkSuite(QObject* parent = nullptr);
    ~UltraPerformanceBenchmarkSuite();
    
    /**
     * @brief Initialize benchmark suite
     */
    bool initialize();
    
    /**
     * @brief Run complete benchmark suite
     * @return Overall benchmark results
     */
    benchmark_results run_complete_benchmark();
    
    /**
     * @brief Run individual performance tests
     */
    benchmark_test_result test_peak_fps_performance();
    benchmark_test_result test_sustained_fps_performance();
    benchmark_test_result test_latency_performance();
    benchmark_test_result test_memory_efficiency();
    benchmark_test_result test_cpu_efficiency();
    benchmark_test_result test_concurrent_stream_processing();
    benchmark_test_result test_network_performance();
    benchmark_test_result test_memory_allocator_performance();
    
    /**
     * @brief Stress tests for reliability
     */
    benchmark_test_result stress_test_extended_processing();
    benchmark_test_result stress_test_memory_pressure();
    benchmark_test_result stress_test_concurrent_load();
    
    /**
     * @brief Validate 10.0/10.0 performance score achievement
     * @return true if perfect score achieved
     */
    bool validate_perfect_performance_score();
    
    /**
     * @brief Generate detailed performance report
     */
    std::string generate_performance_report() const;
    
    /**
     * @brief Export results to JSON format
     */
    std::string export_results_json() const;
    
    /**
     * @brief Compare with baseline performance
     */
    struct baseline_comparison {
        double baseline_fps{1002405.0};
        double baseline_memory_mb{100.0};
        double baseline_latency_us{7.0};
        
        double fps_improvement_factor{0.0};
        double memory_reduction_factor{0.0};
        double latency_improvement_factor{0.0};
        bool exceeds_all_baseline_targets{false};
    };
    
    baseline_comparison compare_with_baseline() const;

signals:
    /**
     * @brief Progress updates during benchmarking
     */
    void benchmark_progress(const QString& test_name, int progress_percent);
    
    /**
     * @brief Individual test completion
     */
    void test_completed(const QString& test_name, bool passed, double result);
    
    /**
     * @brief Perfect performance achievement
     */
    void perfect_performance_achieved(double score);
    
    /**
     * @brief Benchmark suite completion
     */
    void benchmark_completed(const benchmark_results& results);

private:
    /**
     * @brief Generate test ETI frames
     */
    void generate_test_data();
    
    /**
     * @brief Measure system performance baseline
     */
    void measure_system_baseline();
    
    /**
     * @brief Warm up system for consistent benchmarking
     */
    void warmup_system();
    
    /**
     * @brief Monitor system resources during tests
     */
    void start_system_monitoring();
    void stop_system_monitoring();
    
    /**
     * @brief Calculate overall performance score
     */
    double calculate_overall_score() const;
    
    /**
     * @brief Validate individual performance targets
     */
    bool validate_fps_target(double measured_fps) const;
    bool validate_memory_target(double measured_memory_mb) const;
    bool validate_latency_target(double measured_latency_us) const;
    bool validate_concurrency_target(size_t concurrent_streams) const;
    bool validate_cpu_target(double cpu_percent) const;
    
    /**
     * @brief System information collection
     */
    std::string collect_system_info() const;
    
    /**
     * @brief Performance optimization verification
     */
    bool verify_simd_optimizations() const;
    bool verify_memory_optimizations() const;
    bool verify_network_optimizations() const;
    bool verify_threading_optimizations() const;
    
    // Utility methods
    std::chrono::nanoseconds measure_execution_time(std::function<void()> func);
    double measure_cpu_utilization(std::function<void()> func, std::chrono::seconds duration);
    size_t measure_memory_usage();
    
    // Test data generators
    std::vector<uint8_t> generate_valid_eti_frame(uint32_t frame_number);
    std::vector<uint8_t> generate_thai_dab_eti_frame(uint32_t frame_number);
    
    // Performance monitoring
    struct system_monitor {
        std::atomic<bool> monitoring{false};
        std::atomic<double> cpu_usage{0.0};
        std::atomic<size_t> memory_usage{0};
        std::atomic<size_t> network_throughput{0};
        std::thread monitor_thread;
    } system_monitor_;
    
    void monitoring_thread_func();
};

/**
 * @brief Factory function for creating benchmark suite
 */
std::unique_ptr<UltraPerformanceBenchmarkSuite> create_benchmark_suite();

/**
 * @brief Quick performance validation (for CI/CD)
 */
struct quick_validation_result {
    bool performance_targets_met{false};
    double performance_score{0.0};
    std::vector<std::string> failed_tests;
    std::chrono::seconds validation_time{0};
};

/**
 * @brief Run quick performance validation
 * @param duration Test duration (default: 10 seconds)
 * @return Quick validation results
 */
quick_validation_result run_quick_performance_validation(
    std::chrono::seconds duration = std::chrono::seconds{10});

/**
 * @brief Automated performance regression testing
 */
struct regression_test_result {
    bool performance_regression_detected{false};
    double current_score{0.0};
    double reference_score{0.0};
    std::vector<std::string> performance_regressions;
    std::vector<std::string> performance_improvements;
};

/**
 * @brief Run performance regression test
 * @param reference_results Previous benchmark results for comparison
 * @return Regression test results
 */
regression_test_result run_performance_regression_test(
    const benchmark_results& reference_results);

/**
 * @brief Continuous performance monitoring
 */
class ContinuousPerformanceMonitor : public QObject {
    Q_OBJECT

private:
    std::unique_ptr<UltraPerformanceBenchmarkSuite> benchmark_suite_;
    std::atomic<bool> monitoring_active_{false};
    std::thread monitoring_thread_;
    std::chrono::seconds monitoring_interval_{60};

public:
    explicit ContinuousPerformanceMonitor(QObject* parent = nullptr);
    ~ContinuousPerformanceMonitor();
    
    /**
     * @brief Start continuous monitoring
     */
    void start_monitoring(std::chrono::seconds interval = std::chrono::seconds{60});
    
    /**
     * @brief Stop continuous monitoring
     */
    void stop_monitoring();
    
    /**
     * @brief Get latest performance metrics
     */
    benchmark_results get_latest_metrics() const;

signals:
    /**
     * @brief Performance metrics update
     */
    void performance_metrics_updated(const benchmark_results& results);
    
    /**
     * @brief Performance degradation alert
     */
    void performance_degradation_detected(const QString& alert_message);
    
    /**
     * @brief Perfect performance maintenance
     */
    void perfect_performance_maintained(double score);

private:
    void monitoring_loop();
};

} // namespace eti::performance