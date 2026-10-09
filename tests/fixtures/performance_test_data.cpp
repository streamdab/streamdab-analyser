/**
 * @file performance_test_data.cpp
 * @brief Performance test data fixtures for network testing
 */

#include "performance_test_data.h"

namespace test_fixtures {
namespace performance {

const std::vector<NetworkTestScenario> NETWORK_TEST_SCENARIOS = {
    {
        "Low Latency Streaming",
        "239.192.0.1",
        9200,
        1536000,  // 1.536 Mbps
        10.0,     // 10ms target latency
        99.9      // 99.9% reliability
    },
    {
        "High Bitrate Test",
        "239.192.0.2", 
        9201,
        3072000,  // 3.072 Mbps
        25.0,     // 25ms acceptable latency
        99.5      // 99.5% reliability
    },
    {
        "Standard DAB Stream",
        "239.192.0.10",
        9210,
        1152000,  // 1.152 Mbps
        50.0,     // 50ms standard latency
        98.0      // 98% reliability minimum
    }
};

const std::vector<PerformanceBenchmark> PERFORMANCE_BENCHMARKS = {
    {
        "ETI Frame Processing",
        7482.0,        // target_fps
        4.0,           // memory_mb
        0.13,          // cpu_usage_percent
        1000           // duration_ms
    },
    {
        "Service Discovery",
        28256.0,       // operations per second
        2.5,           // memory_mb
        0.08,          // cpu_usage_percent  
        500            // duration_ms
    },
    {
        "Real-time Analysis",
        60.0,          // ui_fps
        150.0,         // memory_mb
        15.0,          // cpu_usage_percent
        30000          // duration_ms (30 seconds)
    }
};

} // namespace performance
} // namespace test_fixtures