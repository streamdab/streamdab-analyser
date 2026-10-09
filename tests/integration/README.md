# TDD Integration Tests Directory

This directory contains integration tests for the ETI Stream Analyser TDD framework.

## Purpose

Integration tests verify that components work together correctly:
- **Component Integration**: Test interactions between multiple components
- **End-to-End Workflows**: Validate complete user scenarios
- **System-Level Validation**: Ensure the entire system functions as expected

## Integration Test Categories

```
integration/
├── test_eti_pipeline.cpp        # Complete ETI processing pipeline
├── test_gui_integration.cpp     # GUI-backend integration
├── test_file_processing.cpp     # File I/O integration
├── test_network_streaming.cpp   # Network streaming integration
├── test_audio_pipeline.cpp      # Audio processing pipeline
└── test_configuration.cpp       # Configuration management integration
```

## TDD Integration Strategy

### Red Phase - Integration Failures
```cpp
TEST(EtiPipelineIntegration, CompleteProcessingWorkflow) {
    // Arrange - Setup complete pipeline
    EtiFileReader reader;
    EtiProcessor processor;
    ServiceManager service_manager;
    AudioDecoder audio_decoder;
    
    // Act - Process complete workflow
    auto eti_data = reader.load("test_data/sample.eti");
    auto processed = processor.process(eti_data);
    auto services = service_manager.extract_services(processed);
    auto audio = audio_decoder.decode(services[0]);
    
    // Assert - Initially fails due to missing integration
    EXPECT_TRUE(eti_data.valid);
    EXPECT_TRUE(processed.success);
    EXPECT_GT(services.size(), 0);
    EXPECT_TRUE(audio.valid);
}
```

### Green Phase - Working Integration
```cpp
TEST(EtiPipelineIntegration, WorkingProcessingWorkflow) {
    // Arrange - Setup integrated components
    EtiProcessingPipeline pipeline; // Integrated implementation
    
    // Act - Process through pipeline
    auto result = pipeline.process_file("test_data/sample.eti");
    
    // Assert - Now passes with proper integration
    EXPECT_TRUE(result.success);
    EXPECT_GT(result.services.size(), 0);
    EXPECT_TRUE(result.audio_available);
}
```

## Integration Test Scenarios

### File Processing Integration
- Load ETI file → Process stream → Extract services → Decode audio
- Validate file format support (ETI, EDI)
- Test large file handling and streaming

### GUI Integration
- User interface → Backend processing → Display updates
- Real-time data visualization
- User interaction handling

### Network Integration
- Network reception → Stream processing → Service extraction
- Protocol handling (HTTP, UDP, TCP)
- Connection management and recovery

### Configuration Integration
- Settings management → Component configuration → Runtime behavior
- Persistence and loading
- Default value handling

## Real-World Test Data

Integration tests use realistic test data:
- **Sample ETI Files**: Real DAB/DAB+ ensemble recordings
- **Multi-Service Streams**: Multiple audio/data services
- **Error Scenarios**: Corrupted or incomplete data
- **Performance Data**: High-volume streams for stress testing

## Test Environment Setup

```cpp
class IntegrationTestEnvironment : public ::testing::Environment {
public:
    void SetUp() override {
        // Initialize test database
        test_db.initialize();
        
        // Setup test configuration
        test_config.load("integration_test_config.yaml");
        
        // Prepare test data
        test_data_manager.prepare_all_fixtures();
    }
    
    void TearDown() override {
        // Cleanup test environment
        test_db.cleanup();
        test_data_manager.cleanup();
    }
};
```

## Quality Requirements

- **Test Isolation**: Each test can run independently
- **Deterministic Results**: Same inputs produce same outputs
- **Realistic Scenarios**: Tests reflect real-world usage
- **Performance Validation**: Integration tests meet performance requirements
- **Error Handling**: Tests verify proper error handling and recovery

## Execution Strategy

### Local Development
```bash
# Run integration tests
make run_integration_tests

# Run specific integration test
./eti_analyser_integration_tests --gtest_filter=*EtiPipeline*
```

### CI/CD Pipeline
```bash
# Full integration test suite
ctest -R integration

# Integration test with coverage
make integration_tests_with_coverage
```

## Test Data Management

- **Version Control**: Test data versioned with code
- **Size Management**: Large files stored with Git LFS
- **Data Generation**: Scripts to generate additional test data
- **Validation**: Test data validated for correctness

## Success Criteria

Integration tests pass when:
- All component interactions work correctly
- End-to-end workflows complete successfully
- Performance requirements are met
- Error scenarios are handled properly
- Configuration changes are applied correctly