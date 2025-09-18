# ETI Stream Analyzer - Development Workflow

## 🎯 Overview

This document outlines the professional development practices and workflows for the ETI Stream Analyzer project, ensuring consistent code quality, cross-platform compatibility, and adherence to broadcast industry standards.

## 🔍 IDE Diagnostic Resolution Practice

### **Core Principle: Zero-Warning Policy**

All code must compile cleanly with **zero errors and zero warnings** before being committed. This ensures:
- Cross-platform compatibility
- Professional code quality
- Maintainability and reliability
- Compliance with broadcast industry standards

### **Standard Resolution Process**

```bash
# 1. Initial Diagnostic Check
pm-agent: communicate with [IDE] to resolve project diagnostic errors/warnings

# 2. Build Status Verification
cd /path/to/streamdab-analyzer
cmake --build . 2>&1

# Expected output for clean build:
# [100%] Built target ETIStreamAnalyzer
# Exit code: 0 (no errors or warnings)
```

### **Systematic Error Resolution by Priority**

#### **🔴 HIGH PRIORITY - Build-Breaking Errors**
These must be fixed immediately as they prevent compilation:

1. **Compilation Errors**
   ```bash
   # Example: Missing includes
   error: use of undeclared identifier 'QDirIterator'
   # Fix: Add #include <QDirIterator>
   
   # Example: Type mismatches  
   error: no matching constructor for initialization of 'QDirIterator'
   # Fix: Check Qt documentation for correct constructor signature
   ```

2. **Header/Implementation Mismatches**
   ```bash
   # Example: Method signature mismatch
   error: out-of-line definition of 'methodName' does not match any declaration
   # Fix: Ensure header and implementation signatures match exactly
   ```

3. **Type Conversion Errors**
   ```bash
   # Example: Qt type mismatch
   error: no matching function for call to 'min'
   # Fix: Use proper type casting or Qt-specific types
   int index2 = std::min(index1 + 1, static_cast<int>(input.size()) - 1);
   ```

#### **🟡 MEDIUM PRIORITY - Code Quality Warnings**
These should be fixed to maintain professional standards:

1. **Unused Parameter Warnings**
   ```cpp
   // For placeholder implementations
   Q_UNUSED(parameter) // Qt macro for future implementation
   // OR
   (void)parameter;    // Standard C++ approach
   ```

2. **Type Conversion Warnings**
   ```cpp
   // Use explicit casting for Qt types
   int size = static_cast<int>(qtContainer.size());
   ```

3. **Missing Virtual Destructors**
   ```cpp
   // For base classes with virtual methods
   virtual ~BaseClass() = default;
   ```

#### **🟢 LOW PRIORITY - Code Cleanup**
These improve code maintainability:

1. **Unused Variables**
   ```cpp
   // Remove or comment out unused variables
   // const int UNUSED_CONSTANT = 42; // Remove if not needed
   ```

2. **Redundant Includes**
   ```cpp
   // Remove includes that are not actually used
   // #include <unnecessary_header.h> // Remove
   ```

### **Platform-Specific Considerations**

#### **Qt Framework Integration**
```cpp
// Use Qt types for consistency
qsizetype size = container.size();  // Not size_t
QString text = "content";           // Not std::string for UI
QByteArray data = getData();        // Not std::vector<uint8_t> for Qt APIs
```

#### **Cross-Platform Type Safety**
```cpp
// Avoid platform-dependent sizes
int index = static_cast<int>(list.size());  // Explicit conversion
```

## 🔧 Development Workflow Integration

### **Pre-Commit Checklist**
```bash
# 1. Build check
cmake --build . 2>&1
[ $? -eq 0 ] || echo "❌ Build failed - fix errors before commit"

# 2. Test execution (when available)
# cmake --build . --target test

# 3. Code formatting (if configured)
# clang-format -i src/**/*.cpp src/**/*.h

# 4. Final verification
cmake --build . 2>&1 | grep -E "(warning|error)" 
# Should produce no output
```

### **IDE Configuration Recommendations**

#### **Zed Editor**
```json
// .zed/settings.json
{
  "language_servers": {
    "clangd": {
      "initialization_options": {
        "clang-tidy": {
          "checks": "*"
        }
      }
    }
  }
}
```

#### **VS Code**
```json
// .vscode/c_cpp_properties.json
{
  "configurations": [{
    "name": "ETI Analyzer",
    "includePath": [
      "${workspaceFolder}/src/**",
      "/opt/homebrew/include/QtCore",
      "/opt/homebrew/include/QtWidgets"
    ],
    "compilerPath": "/usr/bin/clang++",
    "cStandard": "c17",
    "cppStandard": "c++17"
  }]
}
```

## 📚 Common Diagnostic Patterns

### **Qt-Specific Issues**

1. **Missing MOC Generation**
   ```bash
   # Ensure Q_OBJECT macro is present and CMake handles MOC
   # CMakeLists.txt should have:
   set(CMAKE_AUTOMOC ON)
   ```

2. **Signal/Slot Connection Issues**
   ```cpp
   // Use new connection syntax
   connect(sender, &SenderClass::signal, 
           receiver, &ReceiverClass::slot);
   ```

3. **Qt Container Type Mismatches**
   ```cpp
   // Use Qt containers with Qt APIs
   QList<QString> items;        // Not std::vector<std::string>
   QMap<QString, QVariant> map; // Not std::map<std::string, boost::any>
   ```

### **C++ Standard Library Issues**

1. **Template Type Deduction**
   ```cpp
   // Be explicit with template parameters
   auto result = std::min<int>(a, static_cast<int>(b));
   ```

2. **Include Dependencies**
   ```cpp
   #include <algorithm>  // For std::min, std::max
   #include <numeric>    // For std::accumulate
   #include <memory>     // For std::unique_ptr
   ```

## 🎯 Success Metrics

### **Build Quality Indicators**
- ✅ **Zero compilation errors**
- ✅ **Zero warnings** 
- ✅ **Exit code 0** from cmake build
- ✅ **All platforms supported** (Linux, macOS, Windows)
- ✅ **IDE integration** works smoothly

### **Professional Standards**
- ✅ **ETSI compliance** maintained during diagnostic fixes
- ✅ **Broadcast industry practices** followed
- ✅ **Code maintainability** improved
- ✅ **Cross-platform portability** ensured

## 📝 Documentation Updates

When resolving diagnostic issues, ensure documentation reflects:

1. **New dependencies** or includes added
2. **API changes** that affect public interfaces  
3. **Platform-specific considerations** discovered
4. **Best practices** learned during resolution

## 🚀 Automation Opportunities

### **CI/CD Integration**
```yaml
# .github/workflows/diagnostic-check.yml
name: Diagnostic Check
on: [push, pull_request]
jobs:
  build:
    runs-on: ubuntu-latest
    steps:
    - uses: actions/checkout@v3
    - name: Build with diagnostic check
      run: |
        cmake -B build
        cmake --build build 2>&1 | tee build.log
        ! grep -E "(warning|error)" build.log
```

### **Pre-commit Hooks**
```bash
# .git/hooks/pre-commit
#!/bin/bash
cmake --build . 2>&1 | grep -E "(warning|error)" 
if [ $? -eq 0 ]; then
    echo "❌ Diagnostic errors found - commit blocked"
    exit 1
fi
echo "✅ Clean build - commit allowed"
```

---

*This workflow ensures the ETI Stream Analyzer maintains professional broadcast industry standards while providing a smooth development experience across all supported platforms and IDEs.*