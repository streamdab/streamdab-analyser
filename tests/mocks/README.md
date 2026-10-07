# TDD Mock Objects Directory

This directory contains mock implementations for the ETI Stream Analyser TDD framework.

## Purpose

Mock objects enable isolated unit testing by replacing dependencies with controlled implementations:
- **Dependency Isolation**: Test units in isolation from their dependencies
- **Behavior Verification**: Verify interactions between components
- **Test Control**: Simulate various scenarios (success, failure, edge cases)

## Mock Categories

```
mocks/
├── eti_stream_mocks/      # Mock ETI stream processors
├── audio_decoder_mocks/   # Mock audio decoding components
├── network_mocks/         # Mock network/file I/O operations
├── gui_mocks/             # Mock Qt GUI components
└── system_mocks/          # Mock system-level dependencies
```

## TDD Integration with Google Mock

All mocks use Google Mock framework for consistent behavior:

```cpp
#include <gmock/gmock.h>

class MockEtiProcessor : public EtiProcessorInterface {
public:
    MOCK_METHOD(ProcessResult, process, (const EtiStream& stream), (override));
    MOCK_METHOD(bool, isValid, (const EtiFrame& frame), (const, override));
    MOCK_METHOD(void, reset, (), (override));
};
```

## TDD Usage Patterns

### Red Phase - Failing Tests
```cpp
TEST(ServiceManagerTest, HandleProcessingFailure) {
    // Arrange - Setup failing mock
    MockEtiProcessor mock_processor;
    EXPECT_CALL(mock_processor, process(testing::_))
        .WillOnce(testing::Return(ProcessResult{false, "Processing failed"}));
    
    ServiceManager manager(&mock_processor);
    
    // Act & Assert - Test should initially fail
    auto result = manager.processStream(test_stream);
    EXPECT_FALSE(result.success); // This will FAIL until implementation exists
}
```

### Green Phase - Passing Tests
```cpp
TEST(ServiceManagerTest, HandleProcessingSuccess) {
    // Arrange - Setup successful mock
    MockEtiProcessor mock_processor;
    EXPECT_CALL(mock_processor, process(testing::_))
        .WillOnce(testing::Return(ProcessResult{true, "Success"}));
    
    ServiceManager manager(&mock_processor);
    
    // Act & Assert - Test should now pass
    auto result = manager.processStream(test_stream);
    EXPECT_TRUE(result.success); // Passes when implementation is correct
}
```

## Mock Design Principles

1. **Interface-Based**: All mocks implement well-defined interfaces
2. **Behavior-Focused**: Mock the behavior, not the implementation
3. **Minimal**: Only mock what's necessary for the test
4. **Deterministic**: Same inputs always produce same outputs
5. **Verifiable**: Enable verification of interactions

## Quality Standards

- All mocks must compile with Google Mock
- Mock interfaces must match production interfaces exactly
- Include both success and failure scenarios
- Document expected interaction patterns
- Provide factory methods for common configurations