/**
 * @file performance_test_data.h
 * @brief Performance test data fixtures for network testing
 */

#pragma once

#include <vector>
#include <string>

struct NetworkTestScenario {
    std::string name;
    std::string multicast_address;
    int port;
    int bitrate_bps;
    double target_latency_ms;
    double reliability_percent;
};

struct PerformanceBenchmark {
    std::string name;
    double target_fps;
    double memory_mb;
    double cpu_usage_percent;
    int duration_ms;
};

namespace test_fixtures {
namespace performance {

extern const std::vector<NetworkTestScenario> NETWORK_TEST_SCENARIOS;
extern const std::vector<PerformanceBenchmark> PERFORMANCE_BENCHMARKS;

} // namespace performance
} // namespace test_fixtures