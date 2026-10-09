# Phase 1-3 Build Validation Checklist & Performance Baselines

## Build Validation Checklist

### ✅ **Phase 1: Modern ETI Core Engine**

#### Compilation Requirements
- [ ] **Modern ETI Frame Parser** compiles without errors
- [ ] **Performance Processing Config** structures are properly defined
- [ ] **Qt6 Signal/Slot Integration** compiles and links correctly
- [ ] **C++20 Features** (concepts, ranges, structured bindings) work properly
- [ ] **Thread Pool Integration** compiles with QThreadPool support

#### Runtime Requirements
- [ ] **Parser Initialization** succeeds with valid configuration
- [ ] **ETI Frame Processing** handles 6144-byte frames correctly
- [ ] **FIG Analysis** extracts and processes FIG blocks
- [ ] **Performance Metrics** collection and reporting works
- [ ] **Signal Emission** for frameProcessed, figAnalysisComplete

#### Performance Baselines
- **Target FPS**: ≥900 frames/second
- **Frame Processing Time**: ≤1000μs per frame
- **Memory Usage**: ≤100MB operational footprint
- **Initialization Time**: ≤500ms
- **Signal Latency**: ≤1ms from processing to signal emission

### ✅ **Phase 2: ZeroMQ ETI Client**

#### Compilation Requirements
- [ ] **ZeroMQ Integration** links with libzmq correctly
- [ ] **Connection Configuration** structures compile properly
- [ ] **Worker Thread Implementation** compiles with Qt threading
- [ ] **Frame Parser Integration** connects to Modern ETI Core Engine
- [ ] **Performance Statistics** collection compiles

#### Runtime Requirements
- [ ] **ZMQ Context Creation** succeeds without errors
- [ ] **Socket Configuration** applies all settings correctly
- [ ] **Connection Status Tracking** updates properly
- [ ] **Frame Reception** handles ETI frames correctly
- [ ] **Auto-Reconnection** functionality works as expected

#### Performance Baselines
- **Connection Time**: ≤2000ms to establish ZMQ connection
- **Frame Reception Rate**: ≥900 frames/second sustained
- **Latency**: ≤50ms from ZMQ receive to signal emission
- **Memory Usage**: ≤20MB for ZMQ client operations
- **Connection Reliability**: ≥99% uptime during testing

### ✅ **Phase 3a: ETI-NI Processor with TIST**

#### Compilation Requirements
- [ ] **TIST Extraction Logic** compiles without errors
- [ ] **Timestamp Processing** handles all TIST types
- [ ] **Synchronization Algorithms** compile with chrono support
- [ ] **ETI-NI Validation** structures are properly defined
- [ ] **Compliance Checking** integrates with validation framework

#### Runtime Requirements
- [ ] **TIST Extraction** works from LIDATA field correctly
- [ ] **Timestamp Type Detection** identifies all 4 TIST types
- [ ] **Synchronization** achieves target accuracy
- [ ] **Drift Compensation** calculates and reports clock drift
- [ ] **ETI-NI Compliance** validates frame structure

#### Performance Baselines
- **TIST Extraction Time**: ≤10μs per frame
- **Synchronization Accuracy**: ≤1000μs for valid TIST
- **Processing Latency**: ≤100μs additional overhead
- **Memory Usage**: ≤10MB for synchronization state
- **Compliance Validation**: ≤50μs per frame

### ✅ **Phase 3b: Reed-Solomon Codec**

#### Compilation Requirements
- [ ] **Galois Field Mathematics** compiles correctly
- [ ] **Polynomial Operations** work with C++20 features
- [ ] **Codec Parameters** structures are properly defined
- [ ] **Error Correction Algorithms** compile without warnings
- [ ] **Integration Interfaces** connect to ETI processing

#### Runtime Requirements
- [ ] **Encoding Operations** produce correct codewords
- [ ] **Error Detection** identifies corruption accurately
- [ ] **Error Correction** recovers up to t errors
- [ ] **Uncorrectable Detection** properly identifies limits
- [ ] **Performance Statistics** track correction efficiency

#### Performance Baselines
- **Encoding Speed**: ≥1000 codewords/second for MPEG-TS (204,188)
- **Decoding Speed**: ≥500 codewords/second for error correction
- **Correction Capability**: 100% success for errors ≤t
- **Detection Accuracy**: 100% for uncorrectable errors >t
- **Memory Usage**: ≤5MB for codec operations

## Overall System Integration Requirements

### ✅ **Build System Validation**

#### CMake Configuration
- [ ] **Qt6 Detection** finds all required components
- [ ] **Dependency Management** locates faad2, fftw3, zmq
- [ ] **Compiler Settings** apply C++20 standard correctly
- [ ] **Test Framework** integrates Google Test and Qt Test
- [ ] **Cross-Platform** builds work on Linux/Windows/macOS

#### Compilation Success
- [ ] **Zero Errors** in all Phase 1-3 components
- [ ] **Zero Warnings** for critical code paths
- [ ] **Link Success** all libraries resolve correctly
- [ ] **Executable Generation** produces working binary
- [ ] **Test Compilation** all validation tests build

### ✅ **Runtime Integration Validation**

#### Component Interaction
- [ ] **ETI Parser ↔ ZMQ Client** integration works seamlessly
- [ ] **ZMQ Client ↔ ETI-NI Processor** data flow is correct
- [ ] **ETI-NI ↔ Reed-Solomon** validation pipeline works
- [ ] **Configuration Management** loads/saves all settings
- [ ] **Signal/Slot Connections** propagate events correctly

#### End-to-End Processing
- [ ] **File Input** processes ETI files correctly
- [ ] **Network Input** receives ZMQ streams properly
- [ ] **Real-time Processing** maintains ≥900 FPS
- [ ] **Error Handling** gracefully manages failures
- [ ] **Resource Management** prevents memory leaks

## Performance Validation Benchmarks

### ✅ **Processing Speed Requirements**

| Component | Metric | Target | Validation Method |
|-----------|--------|--------|-------------------|
| Modern ETI Parser | Frame Processing | ≥900 FPS | Process 1000 frames, measure total time |
| ZeroMQ Client | Reception Rate | ≥900 FPS | Simulate ZMQ stream, measure throughput |
| ETI-NI Processor | TIST Extraction | ≤10μs/frame | Process frames with TIST, measure extraction time |
| Reed-Solomon | MPEG-TS Encoding | ≥1000/sec | Encode 1000 packets, measure total time |
| Reed-Solomon | Error Correction | ≥500/sec | Correct errors in 500 codewords, measure time |

### ✅ **Memory Usage Requirements**

| Component | Metric | Target | Validation Method |
|-----------|--------|--------|-------------------|
| Modern ETI Parser | Operational Memory | ≤100MB | Monitor RSS during processing |
| ZeroMQ Client | Connection Memory | ≤20MB | Measure memory after connection |
| ETI-NI Processor | Sync State Memory | ≤10MB | Monitor memory during sync operations |
| Reed-Solomon | Codec Memory | ≤5MB | Measure memory after initialization |
| Total System | Combined Memory | ≤150MB | Full system memory monitoring |

### ✅ **Latency Requirements**

| Processing Stage | Latency Target | Validation Method |
|------------------|----------------|-------------------|
| ETI Frame → Parse Result | ≤1000μs | Measure parseFrame() execution time |
| ZMQ Receive → Signal Emit | ≤50ms | Measure ZMQ reception to Qt signal |
| TIST Extract → Sync Update | ≤100μs | Measure TIST processing overhead |
| Error Detection → Correction | ≤500μs | Measure RS decode operations |
| End-to-End Pipeline | ≤100ms | Total latency from input to output |

### ✅ **Reliability Requirements**

| Component | Reliability Target | Validation Method |
|-----------|-------------------|-------------------|
| ETI Frame Parsing | ≥99.5% success rate | Process 1000 frames, count failures |
| ZMQ Connection | ≥99% uptime | 1-hour stability test |
| TIST Synchronization | ≥95% accuracy achievement | Process 100 frames with valid TIST |
| Reed-Solomon Correction | 100% for errors ≤t | Test all error patterns within capability |
| Configuration Persistence | 100% data integrity | Save/load test with complex configurations |

## Test Execution Protocol

### ✅ **Automated Test Execution**

1. **Build Validation Tests**
   ```bash
   cd /home/seksan/workspace/streamdab-analyser/build
   make -j$(nproc) StreamDAB_Analyser
   echo $? # Should be 0 for success
   ```

2. **Component Unit Tests**
   ```bash
   cd /home/seksan/workspace/streamdab-analyser/build/tests
   ./test_phase_1_3_validation_suite
   ```

3. **Performance Benchmark Tests**
   ```bash
   cd /home/seksan/workspace/streamdab-analyser/build/tests
   ./test_performance_validation
   ```

4. **Integration Test Suite**
   ```bash
   cd /home/seksan/workspace/streamdab-analyser/build/tests
   ./test_e2e_integration
   ```

### ✅ **Manual Validation Steps**

1. **Executable Launch Test**
   ```bash
   cd /home/seksan/workspace/streamdab-analyser/build
   ./StreamDAB_Analyser --version
   ./StreamDAB_Analyser --help
   ```

2. **Configuration File Test**
   ```bash
   # Test configuration loading
   ./StreamDAB_Analyser --config=test_config.ini
   ```

3. **ETI File Processing Test**
   ```bash
   # Test file input processing
   ./StreamDAB_Analyser --input=sample.eti --output=analysis.json
   ```

4. **ZMQ Connection Test**
   ```bash
   # Test network connectivity (requires ODR-DabMux or test server)
   ./StreamDAB_Analyser --zmq-endpoint=tcp://127.0.0.1:9200
   ```

## Success Criteria Summary

### ✅ **Build Success Criteria**
- **Compilation**: 0 errors in all Phase 1-3 components
- **Linking**: Successful executable generation
- **Dependencies**: All libraries (Qt6, ZMQ, FAAD, FFTW) detected and linked
- **Tests**: All validation tests compile and link successfully

### ✅ **Performance Success Criteria**
- **Speed**: ≥900 FPS ETI processing capability demonstrated
- **Memory**: ≤150MB total system memory usage
- **Latency**: ≤100ms end-to-end processing latency
- **Reliability**: ≥99% success rate for core operations

### ✅ **Integration Success Criteria**
- **Component Communication**: All signal/slot connections working
- **Data Flow**: ETI frames flow correctly through entire pipeline
- **Error Handling**: Graceful degradation on invalid input
- **Configuration**: Settings persist and load correctly

### ✅ **Readiness for Phase 4**
- **Stable Foundation**: Phase 1-3 provides reliable base
- **Performance Baseline**: Meets all speed and memory targets
- **API Completeness**: All interfaces ready for GUI integration
- **Test Coverage**: Comprehensive validation ensures quality

## Documentation and Evidence

### ✅ **Required Evidence for Build Manager Agent**
1. **Compilation Logs**: Clean build output with 0 errors
2. **Test Results**: All validation tests passing
3. **Performance Reports**: Benchmark results meeting targets
4. **Memory Profiles**: Usage reports within limits
5. **Integration Tests**: End-to-end pipeline working correctly

### ✅ **Deliverables for User Validation**
1. **Working Executable**: Functional StreamDAB_Analyser binary
2. **Test Reports**: Comprehensive validation evidence
3. **Performance Metrics**: Documented benchmark results
4. **Configuration Examples**: Sample .ini files for testing
5. **Integration Examples**: ZMQ connection and ETI file processing demos

---

**Status**: Ready for Build Manager Agent execution once compilation issues are resolved.
**Next Phase**: GUI integration and professional UI implementation (Phase 4).
**Quality Gate**: All checklists must be ✅ before proceeding to Phase 4.