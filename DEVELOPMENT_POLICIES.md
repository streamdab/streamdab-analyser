# Development Policies - ETI Stream Analyzer

## 🚨 **CRITICAL POLICY: NO MOCK/PLACEHOLDER IMPLEMENTATIONS**

**Established:** September 19, 2025  
**Authority:** PM Agent Policy Decision  
**Enforcement:** MANDATORY for all development

### **Policy Statement**

> **ALL functions and methods MUST be real, working implementations. NO mock, placeholder, or commented-out functionality is permitted in production code.**

### **Policy Background**

This policy was established after a critical regression where ETI processor initialization was commented out, causing the application to fail with "ETI processor is not initialized" errors. This type of issue violates user expectations and professional standards.

### **Prohibited Practices**

❌ **NEVER comment out critical initialization code**
```cpp
// BAD - POLICY VIOLATION
// m_etiProcessor = std::make_unique<EtiProcessor>();
// if (!m_etiProcessor->initialize()) {
//     return false;
// }
```

❌ **NEVER use placeholder implementations**
```cpp
// BAD - POLICY VIOLATION
bool EtiProcessor::initialize() {
    // TODO: Implement later
    return true;  // Fake success
}
```

❌ **NEVER disable core functionality without proper fallbacks**
```cpp
// BAD - POLICY VIOLATION
bool processFile(const QString& filename) {
    // TEMPORARILY DISABLED DUE TO ISSUE
    return false;
}
```

### **Required Practices**

✅ **Real implementations with proper error handling**
```cpp
// GOOD - POLICY COMPLIANT
m_etiProcessor = std::make_unique<EtiProcessor>();
if (!m_etiProcessor->initialize()) {
    Logger::instance().log(Logger::Error, "MainWindow", "Failed to initialize ETI processor");
    return false;
}
```

✅ **Proper error states instead of commenting out**
```cpp
// GOOD - POLICY COMPLIANT
bool EtiProcessor::initialize() {
    if (!initializeBackend()) {
        m_lastError = "Backend initialization failed";
        return false;
    }
    m_initialized = true;
    return true;
}
```

✅ **Graceful degradation with clear messaging**
```cpp
// GOOD - POLICY COMPLIANT  
bool processFile(const QString& filename) {
    if (!isFeatureAvailable()) {
        emit errorOccurred("Feature currently unavailable - see documentation");
        return false;
    }
    // ... real implementation
}
```

## 🧪 **MANDATORY TESTING REQUIREMENTS**

### **Initialization Tests**

Every major component MUST have initialization tests that verify:

1. **Component can be created successfully**
2. **Initialization method succeeds**  
3. **Component reports proper initialized state**
4. **Core functionality is available after initialization**

### **Regression Prevention Tests**

```cpp
// EXAMPLE: Required test pattern
TEST_F(ComponentTest, MustBeInitializedAfterSetup) {
    // ARRANGE
    auto component = std::make_unique<Component>();
    
    // ACT
    bool success = component->initialize();
    
    // ASSERT - ENFORCE POLICY
    EXPECT_TRUE(success) << "Component initialization failed - check for commented code";
    EXPECT_TRUE(component->isInitialized()) << "Component not properly initialized";
    EXPECT_NE(component->getCoreSubsystem(), nullptr) 
        << "POLICY VIOLATION: Core subsystem not created - check for disabled initialization";
}
```

### **Anti-Mock Verification Tests**

```cpp
// EXAMPLE: Verify real implementation
TEST_F(ComponentTest, HasRealImplementationNotMock) {
    auto component = std::make_unique<Component>();
    component->initialize();
    
    // Test that methods return meaningful results (not placeholders)
    QString status = component->getStatus();
    EXPECT_FALSE(status.isEmpty()) << "getStatus should not return empty (mock behavior)";
    EXPECT_NE(status, "TODO") << "getStatus should not be placeholder";
    EXPECT_NE(status, "Not implemented") << "getStatus should not be mock";
}
```

## 🔍 **CODE REVIEW REQUIREMENTS**

### **Mandatory Review Checklist**

Before any code merge, reviewers MUST verify:

- [ ] **No commented-out initialization code**
- [ ] **No placeholder return values (true/false without logic)**
- [ ] **No "TODO" or "Not implemented" methods in critical paths**
- [ ] **All initialization methods have real implementations**
- [ ] **Error handling properly implemented (not just return false)**
- [ ] **Tests verify real functionality, not mock behavior**

### **Immediate Red Flags**

🚩 **STOP - Reject immediately if found:**
- Commented initialization in constructors/setup methods
- Methods returning hardcoded success/failure without logic
- "Temporarily disabled" functionality without proper fallbacks
- Missing initialization of core subsystems

## 🏗️ **BUILD & CI POLICIES**

### **Automatic Policy Enforcement**

1. **Unit tests MUST pass** - No exceptions
2. **Integration tests MUST verify real functionality**
3. **Code coverage for initialization paths MUST be >90%**
4. **Static analysis tools to detect commented critical code**

### **Pre-commit Hooks**

Required automated checks:
- Scan for commented initialization patterns
- Verify test coverage on modified initialization code
- Check for placeholder method implementations

## 📋 **INCIDENT RESPONSE**

### **When Policy Violations Are Found**

1. **IMMEDIATE**: Create failing test that reproduces the issue
2. **URGENT**: Fix the actual implementation (not just uncomment)
3. **REQUIRED**: Add regression tests to prevent recurrence
4. **REVIEW**: Audit related code for similar violations

### **Post-Incident Actions**

- Update test suite to catch similar patterns
- Review code review process effectiveness
- Document lessons learned
- Update static analysis rules

## 🎯 **SUCCESS METRICS**

### **Policy Compliance Indicators**

- ✅ Zero "initialization failed" runtime errors
- ✅ All core subsystems report properly initialized
- ✅ Integration tests pass without mock dependencies
- ✅ User functionality works as documented

### **Quality Gates**

- **Bronze**: No commented critical initialization code
- **Silver**: All initialization paths tested and verified  
- **Gold**: Proactive detection and prevention of policy violations
- **Platinum**: Zero initialization-related user issues

---

## **ENFORCEMENT AUTHORITY**

This policy is **MANDATORY** and enforced by:
- PM Agent coordination oversight
- Automated testing pipeline
- Code review requirements
- CI/CD quality gates

**Violations result in:**
1. Immediate build failure
2. Required fix before merge
3. Additional testing requirements
4. Code review process review

**Policy Owner:** PM Agent  
**Last Updated:** September 19, 2025  
**Next Review:** October 19, 2025