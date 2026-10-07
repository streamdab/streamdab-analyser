# TDD Performance Benchmarks Directory

This directory contains performance benchmarks for the ETI Stream Analyser TDD framework.

## Purpose

Performance benchmarks ensure that TDD implementation maintains acceptable performance:
- **Performance Regression Detection**: Catch performance degradation early
- **Optimization Validation**: Verify that optimizations actually improve performance
- **Scalability Testing**: Ensure the system scales with real-world data volumes

## Benchmark Categories

```
benchmarks/
├── benchmark_eti_processing.cpp   # ETI stream processing performance
├── benchmark_fic_decoding.cpp     # FIC decoding performance
├── benchmark_audio_decoding.cpp   # Audio processing performance
├── benchmark_gui_updates.cpp      # GUI responsiveness benchmarks
└── benchmark_memory_usage.cpp     # Memory allocation/deallocation
```

## TDD Integration with Google Benchmark

All benchmarks use Google Benchmark framework:

```cpp
#include <benchmark/benchmark.h>
#include "core/eti_processor.h"

static void BM_EtiProcessing(benchmark::State& state) {
    EtiProcessor processor;
    auto test_stream = load_test_stream(state.range(0)); // Parameterized by size
    
    for (auto _ : state) {
        auto result = processor.process(test_stream);
        benchmark::DoNotOptimize(result);
    }
    
    state.SetItemsProcessed(state.iterations() * state.range(0));
    state.SetBytesProcessed(state.iterations() * test_stream.size());
}

BENCHMARK(BM_EtiProcessing)
    ->Range(1024, 8<<20)  // 1KB to 8MB
    ->Unit(benchmark::kMillisecond);
```

## Performance Requirements

Based on professional broadcast requirements:

### Real-time Processing Requirements
- **ETI Frame Processing**: < 1ms per 6144-byte frame
- **Audio Decoding**: Real-time (48kHz, 16-bit, stereo)
- **FIC Decoding**: < 100μs per FIC block
- **GUI Updates**: 60 FPS responsiveness (< 16.67ms per frame)

### Throughput Requirements
- **ETI Stream Processing**: > 10 Mbps sustained
- **Multi-service Decoding**: Support 20+ concurrent services
- **Memory Usage**: < 100MB for typical ensemble
- **Startup Time**: < 2 seconds to operational state

## TDD Performance Testing Strategy

### Red Phase - Performance Baselines
```cpp
TEST(PerformanceTest, EtiProcessingMeetsRequirements) {
    EtiProcessor processor;
    auto start = std::chrono::high_resolution_clock::now();
    
    auto result = processor.process(standard_eti_frame);
    
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    
    // Initially fails - no optimized implementation
    EXPECT_LT(duration.count(), 1000); // < 1ms requirement
}
```

### Green Phase - Meet Performance Goals
```cpp
TEST(PerformanceTest, OptimizedEtiProcessing) {
    OptimizedEtiProcessor processor; // Optimized implementation
    auto start = std::chrono::high_resolution_clock::now();
    
    auto result = processor.process(standard_eti_frame);
    
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    
    // Now passes with optimized implementation
    EXPECT_LT(duration.count(), 1000);
    EXPECT_TRUE(result.success);
}
```

## Benchmark Execution

### Running Benchmarks
```bash
# Run all benchmarks
make run_benchmarks

# Run specific benchmark
./eti_analyser_benchmarks --benchmark_filter=BM_EtiProcessing

# Generate benchmark report
./eti_analyser_benchmarks --benchmark_format=json > benchmark_results.json
```

### Performance Monitoring
```bash
# Compare benchmark results over time
benchmark_compare.py baseline.json current.json

# Generate performance regression report
performance_report.py --threshold=5% benchmark_results.json
```

## Quality Gates

Performance benchmarks are integrated into TDD quality gates:
- **Performance Regression**: > 5% slowdown blocks commits
- **Memory Regression**: > 10% memory increase blocks commits
- **Startup Time**: > 2 seconds blocks release
- **Real-time Processing**: Failure to meet real-time requirements blocks feature

## Continuous Performance Monitoring

- Benchmarks run automatically in CI/CD pipeline
- Performance trends tracked over time
- Regression alerts sent to development team
- Performance reports generated for each release