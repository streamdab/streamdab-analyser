# TDD Developer Guide - StreamDAB Analyzer

> **🎯 Professional Test-Driven Development Framework**  
> Complete guide for ETI Stream Analyzer TDD workflow with Red-Green-Refactor methodology

## 📋 Quick Reference

| **Command** | **Purpose** | **Usage** |
|-------------|-------------|-----------|
| `make tdd_red_phase` | Run tests expecting failures (RED) | Initial test writing |
| `make tdd_green_phase` | Run tests expecting all to pass (GREEN) | After implementation |
| `make tdd_quality_gate` | Full compliance validation | Before merge |
| `make run_tests` | Run all test suites | General testing |
| `make tdd_coverage_generate` | Generate coverage reports | Coverage analysis |

## 🔄 TDD Workflow - Red-Green-Refactor

### **Phase 1: RED 🔴 - Write Failing Tests First**

```bash
# Set TDD phase environment
export TDD_PHASE=RED

# Build and run tests (should fail)
cd build
make tdd_red_phase
```

**Example: Writing a RED test**
```cpp
TDD_RED_PHASE(NewFeature)
    // Write test that defines expected behavior
    EtiProcessor processor;
    
    // This should fail initially (before implementation)
    bool result = processor.newFeature();
    
    if (qEnvironmentVariableIsSet("TDD_FORCE_RED")) {
        TDD_RED_ASSERT(result, "New feature should fail in RED phase");
    }
}
```

### **Phase 2: GREEN 🟢 - Implement Minimal Code**

```bash
# Set TDD phase environment
export TDD_PHASE=GREEN

# Implement minimal code to pass tests
# Build and run tests (should pass)
make tdd_green_phase
```

**Example: GREEN implementation**
```cpp
TDD_GREEN_PHASE(NewFeature)
    EtiProcessor processor;
    
    // This should pass after minimal implementation
    bool result = processor.newFeature();
    TDD_GREEN_ASSERT(result, "New feature must work in GREEN phase");
    
    // Verify basic functionality
    QVERIFY(processor.isFeatureWorking());
}
```

### **Phase 3: REFACTOR 🔵 - Improve Code Quality**

```bash
# Set TDD phase environment
export TDD_PHASE=REFACTOR

# Refactor code while keeping tests green
make tdd_green_phase  # Tests should still pass
make tdd_coverage_generate  # Check coverage
```

## 🏗️ Project Structure

```
tests/
├── tdd_framework.h          # Core TDD framework
├── tdd_framework.cpp        # TDD implementation
├── tdd_workflow.cmake       # CMake TDD integration
├── core/
│   ├── test_eti_processor.cpp      # Core component tests
│   └── test_modern_frame_parser.cpp
├── gui/
│   ├── test_main_window.cpp        # GUI component tests
│   └── test_*_widget.cpp
├── e2e/
│   └── test_comprehensive_tdd_workflows.cpp  # E2E tests
├── unit/
│   └── test_*.cpp                  # Unit tests
└── scripts/
    └── check_coverage.sh           # Coverage enforcement
```

## 🎯 Test Categories and Standards

### **Unit Tests (CRITICAL Priority)**
```cpp
TDD_TEST_CASE(ComponentTest, UNIT, CRITICAL)
```
- **Coverage Requirement**: 80% minimum
- **Timeout**: 30 seconds
- **Focus**: Individual component functionality
- **Example**: ETI frame parsing, FIG analysis

### **GUI Tests (HIGH Priority)**
```cpp
TDD_TEST_CASE(UITest, GUI, HIGH)
```
- **Coverage Requirement**: 70% minimum
- **Timeout**: 90 seconds
- **Focus**: Qt widget functionality, user interactions
- **Example**: Main window layout, dialog functionality

### **E2E Tests (CRITICAL Priority)**
```cpp
TDD_TEST_CASE(WorkflowTest, E2E, CRITICAL)
```
- **Coverage Requirement**: 75% minimum
- **Timeout**: 120 seconds
- **Focus**: Complete user workflows
- **Example**: File-to-analysis workflow

### **ETSI Compliance Tests**
```cpp
ETSI_COMPLIANCE_TEST(EN_300_799, frame_structure)
```
- **Standard**: ETSI EN 300 799/401
- **Focus**: Broadcast industry compliance
- **Validation**: Frame structure, FIG parsing

## 🚀 Common Development Workflows

### **Adding a New Feature**

1. **Write RED test first**:
```bash
# Create test file
cp tests/core/test_template.cpp tests/core/test_new_feature.cpp

# Write failing test
vim tests/core/test_new_feature.cpp

# Run RED phase
export TDD_PHASE=RED
make tdd_red_phase
```

2. **Implement minimal GREEN solution**:
```bash
# Add feature to header
vim src/core/component.h

# Implement minimal functionality
vim src/core/component.cpp

# Run GREEN phase
export TDD_PHASE=GREEN
make tdd_green_phase
```

3. **REFACTOR and improve**:
```bash
# Improve implementation
vim src/core/component.cpp

# Ensure tests still pass
make tdd_green_phase

# Check coverage
make tdd_coverage_generate
```

### **Performance Testing**

```cpp
void testPerformanceRequirement() {
    // Target: 7,482 FPS for ETI processing
    TDD_BENCHMARK([&]() {
        processor.processFrames(testData);
    }, 133, "Process 1000 ETI frames"); // 133ms max for 1000 frames
}
```

### **Memory Testing**

```cpp
void testMemoryEfficiency() {
    // Target: < 4MB memory usage
    TDD_MEMORY_CHECK([&]() {
        for (int i = 0; i < 10000; ++i) {
            processor.processFrame(frame);
        }
    }, 4, "ETI processing memory usage");
}
```

## 📊 Quality Gates and Coverage

### **Coverage Requirements**

| **Component** | **Minimum Coverage** | **Target** |
|---------------|---------------------|------------|
| Core ETI Processing | 85% | 90% |
| GUI Components | 70% | 80% |
| Utilities | 80% | 85% |
| Overall Project | 80% | 85% |

### **Quality Gate Commands**

```bash
# Core component gate (unit tests only)
make tdd_core_gate

# Integration gate (E2E tests)
make tdd_integration_gate

# Full quality gate (all tests + coverage)
make tdd_full_gate

# Coverage compliance check
make tdd_coverage_check
```

### **Coverage Analysis**

```bash
# Generate coverage report
make tdd_coverage_generate

# Check coverage requirements
./scripts/check_coverage.sh

# View HTML report
open build/coverage_html/index.html
```

## 🔧 Advanced TDD Features

### **Mock Integration**

```cpp
class MockETIProcessor : public QObject {
    Q_OBJECT
public:
    MOCK_METHOD(bool, processFile, (const QString& filename), ());
    MOCK_METHOD(bool, processData, (const QByteArray& data), ());
};
```

### **Signal Testing**

```cpp
void testSignalEmission() {
    TDD_VERIFY_SIGNAL(&processor, &EtiProcessor::frameProcessed, 1);
    
    processor.processFrame(testFrame);
    
    // Signal should be emitted within timeout
    TDD_VERIFY_SIGNAL_TIMEOUT(&processor, &EtiProcessor::serviceDiscovered, 1000);
}
```

### **ETSI Compliance Validation**

```cpp
ETSI_COMPLIANCE_TEST(EN_300_799, sync_pattern)
    QByteArray frame = generateValidETIFrame();
    ETSI_VALIDATE_FRAME(frame);
    
    bool result = parser.parseFrame(frame);
    QVERIFY2(result, "Must parse ETSI compliant frames");
}
```

## 🐛 Debugging and Troubleshooting

### **Common Issues**

1. **Tests failing in GREEN phase**:
```bash
# Check if implementation matches test expectations
make run_unit_tests_only --verbose

# Review test output for specific failures
```

2. **Coverage not meeting requirements**:
```bash
# Generate detailed coverage report
make tdd_coverage_generate

# Review uncovered lines
open build/coverage_html/index.html

# Add tests for uncovered code paths
```

3. **Performance tests failing**:
```bash
# Run performance tests in isolation
make run_performance_tests

# Profile specific operations
valgrind --tool=callgrind ./build/eti_analyzer_unit_tests
```

### **Memory Leak Detection**

```bash
# Run with Valgrind (Linux/macOS)
make tdd_memcheck_all

# Check for memory leaks
make eti_analyzer_unit_tests_memcheck
```

## 📈 Continuous Integration

### **Pre-commit Checks**

```bash
# Full quality validation before commit
make tdd_quality_gate

# Quick verification
make run_tests_quiet
```

### **CI/CD Integration**

```yaml
# .github/workflows/tdd.yml
- name: Run TDD Quality Gates
  run: |
    mkdir build && cd build
    cmake .. -DENABLE_COVERAGE=ON
    make tdd_full_gate
```

## 📝 Best Practices

### **Test Writing Guidelines**

1. **Always write tests first (RED phase)**
2. **Use descriptive test names**: `test_processValidETIFrame_ShouldReturnTrue`
3. **One assertion per test when possible**
4. **Use appropriate test categories (UNIT/GUI/E2E)**
5. **Include performance and memory tests for core components**

### **Implementation Guidelines**

1. **Implement minimal code to pass (GREEN phase)**
2. **Refactor while keeping tests green**
3. **Maintain 80%+ code coverage**
4. **Document complex implementations**
5. **Use ETSI compliance tests for broadcast standards**

### **Quality Assurance**

1. **Run full quality gate before merge**: `make tdd_quality_gate`
2. **Check coverage requirements**: `make tdd_coverage_check`
3. **Validate performance requirements**: `make run_performance_tests`
4. **Test memory efficiency**: `make tdd_memcheck_all`

## 🎯 Success Metrics

- ✅ **All tests pass in GREEN phase**
- ✅ **Coverage meets 80% minimum requirement**
- ✅ **Performance tests validate 7,482 FPS target**
- ✅ **Memory usage stays under 4MB target**
- ✅ **ETSI compliance tests pass**
- ✅ **No memory leaks detected**
- ✅ **E2E workflows complete successfully**

---

## 📚 Additional Resources

- **[ETSI Compliance Framework](ETSI_COMPLIANCE_FRAMEWORK.md)** - Broadcast standards testing
- **[Testing Framework Requirements](TESTING_FRAMEWORK_REQUIREMENTS.md)** - Detailed test specifications
- **[Development Workflow](DEVELOPMENT_WORKFLOW.md)** - General development process
- **[Qt Designer Integration](QT_DESIGNER_DEVELOPMENT_GUIDE.md)** - UI development with TDD

---

**🏆 Remember: Red-Green-Refactor is not just a process, it's a mindset for professional software development.**