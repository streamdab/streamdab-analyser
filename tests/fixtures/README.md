# TDD Test Fixtures Directory

This directory contains test data and fixtures for the ETI Stream Analyser TDD framework.

## Purpose

Test fixtures provide consistent, reusable test data for the TDD Red-Green-Refactor cycle:
- **Red Phase**: Fixtures help create comprehensive failing tests
- **Green Phase**: Fixtures provide the data needed to make tests pass
- **Refactor Phase**: Fixtures ensure tests remain stable during refactoring

## Directory Structure

```
fixtures/
├── eti_streams/           # Sample ETI stream files
├── audio_samples/         # Test audio data
├── fic_data/              # FIC (Fast Information Channel) test data
├── ensemble_configs/      # DAB ensemble configuration files
├── error_scenarios/       # Corrupted/invalid test data
└── performance_data/      # Large datasets for performance testing
```

## TDD Integration

Each fixture should include:
1. **Valid test data** for positive test cases
2. **Invalid test data** for negative test cases  
3. **Edge case data** for boundary testing
4. **Performance data** for benchmark testing

## Usage in Tests

```cpp
// Example usage in TDD tests
TEST(EtiProcessorTest, ProcessValidEtiStream) {
    // Arrange - Load fixture
    auto test_stream = load_fixture("eti_streams/valid_ensemble.eti");
    
    // Act - Execute code under test
    EtiProcessor processor;
    auto result = processor.process(test_stream);
    
    // Assert - Verify results
    EXPECT_TRUE(result.success);
    EXPECT_GT(result.services.size(), 0);
}
```

## Quality Requirements

- All fixtures must be deterministic (same results every time)
- Fixtures should be minimal but comprehensive
- Include both ETSI-compliant and non-compliant data
- Document the expected behavior for each fixture