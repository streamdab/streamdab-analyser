# 🔗 **ETISnoop Integration Plan & Migration Strategy**
*Updated: September 19, 2025 - Critical Requirement Update*

## 🚨 **CRITICAL REQUIREMENT UPDATE**

**ETISnoop Source Specification:**
- **Repository:** [https://github.com/sekz/etisnoop](https://github.com/sekz/etisnoop)
- **Type:** Customized version with DAB Thai standards support
- **Priority:** IMMEDIATE - Required before any other development
- **Integration Method:** Git submodule with CMake build integration

---

## 🎯 **INTEGRATION OVERVIEW**

### **Current State Assessment**
- **Existing Implementation:** Placeholder ETI processing classes with no real functionality
- **Target Implementation:** Proven ETISnoop backend with Thai DAB standards support
- **Migration Scope:** Complete replacement of placeholder code with ETISnoop integration
- **Timeline:** Phase 0 (Immediate) - 1-2 weeks for complete integration

### **Integration Benefits**
- **Proven Stability:** ETISnoop is a mature, tested ETI processing engine
- **Thai DAB Support:** Pre-integrated Thai language and regional standards
- **ETSI Compliance:** Complete implementation of all required ETSI standards
- **Performance:** Optimized for real-time ETI processing and analysis
- **Community Support:** Established codebase with ongoing development

---

## 📋 **PHASE 0: ETISnoop Integration (IMMEDIATE PRIORITY)**
*Timeline: Week 0-1 | Priority: CRITICAL*

### **Step 1: Repository Integration**
**Responsible Agent:** Build Manager Agent + PM Agent
**Timeline:** 1-2 days

```bash
# Add ETISnoop as Git submodule
cd /home/seksan/workspace/streamdab-analyzer
git submodule add https://github.com/sekz/etisnoop.git third_party/etisnoop
git submodule update --init --recursive

# Verify Thai DAB support
cd third_party/etisnoop
git log --grep="Thai" --oneline
git log --grep="DAB" --oneline | head -10
```

**Deliverables:**
- ✅ ETISnoop submodule configured in `third_party/etisnoop/`
- ✅ Git submodule properly initialized and updated
- ✅ Thai DAB support features identified and documented

### **Step 2: Build System Integration**
**Responsible Agent:** Build Manager Agent
**Timeline:** 2-3 days

**CMakeLists.txt Updates:**
```cmake
# Add ETISnoop subdirectory
add_subdirectory(third_party/etisnoop)

# ETISnoop library target
add_library(etisnoop_integration STATIC
    src/core/etisnoop_wrapper.cpp
    src/core/etisnoop_bridge.cpp
)

# Link ETISnoop libraries
target_link_libraries(etisnoop_integration
    PUBLIC
        etisnoop_core          # Main ETISnoop library
        etisnoop_thai          # Thai DAB extensions
        Qt6::Core
        Qt6::Widgets
)

# Include ETISnoop headers
target_include_directories(etisnoop_integration
    PUBLIC
        third_party/etisnoop/src
        third_party/etisnoop/include
)
```

**Deliverables:**
- ✅ CMake build system successfully compiles ETISnoop
- ✅ ETISnoop libraries linked to main application
- ✅ Cross-platform build configuration operational

### **Step 3: C++ Wrapper Development**
**Responsible Agent:** Standards Compliance Agent
**Timeline:** 3-4 days

**ETISnoop Wrapper Interface:**
```cpp
// src/core/etisnoop_wrapper.h
#ifndef ETISNOOP_WRAPPER_H
#define ETISNOOP_WRAPPER_H

#include <QObject>
#include <QString>
#include <QByteArray>
#include "etisnoop/eti_parser.h"   // ETISnoop core
#include "etisnoop/thai_support.h" // Thai DAB extensions

class ETISnoopWrapper : public QObject {
    Q_OBJECT

public:
    explicit ETISnoopWrapper(QObject *parent = nullptr);
    ~ETISnoopWrapper();

    // File processing
    bool openETIFile(const QString &filename);
    bool processETIFrame(const QByteArray &frameData);
    
    // Thai DAB support
    bool enableThaiSupport(bool enable = true);
    QString getThaiServiceLabel(uint32_t serviceId) const;
    
    // Service information
    QStringList getServices() const;
    QString getEnsembleLabel() const;
    
    // Real-time processing
    void startRealTimeProcessing();
    void stopRealTimeProcessing();

signals:
    void frameProcessed(const QString &frameInfo);
    void serviceDiscovered(const QString &serviceName);
    void errorDetected(const QString &errorMessage);
    void thaiServiceFound(const QString &thaiLabel);

private:
    class ETISnoopWrapperPrivate;
    std::unique_ptr<ETISnoopWrapperPrivate> d;
};

#endif // ETISNOOP_WRAPPER_H
```

**Deliverables:**
- ✅ C++ wrapper classes for ETISnoop integration
- ✅ Qt signals/slots interface for GUI communication
- ✅ Thai DAB support interface and validation
- ✅ Error handling and logging integration

### **Step 4: Qt GUI Integration**
**Responsible Agent:** UI/UX Agent
**Timeline:** 2-3 days

**GUI Data Bridge:**
```cpp
// src/gui/etisnoop_bridge.cpp
void MainWindow::connectETISnoopSignals() {
    connect(m_etisnoopWrapper, &ETISnoopWrapper::frameProcessed,
            this, &MainWindow::updateFrameDisplay);
    
    connect(m_etisnoopWrapper, &ETISnoopWrapper::serviceDiscovered,
            m_serviceBrowser, &ServiceBrowser::addService);
    
    connect(m_etisnoopWrapper, &ETISnoopWrapper::thaiServiceFound,
            this, &MainWindow::displayThaiService);
            
    connect(m_etisnoopWrapper, &ETISnoopWrapper::errorDetected,
            m_errorLog, &ErrorLog::addError);
}

void MainWindow::displayThaiService(const QString &thaiLabel) {
    // Ensure proper Thai character rendering
    QFont thaiFont("Noto Sans Thai", 10);
    m_serviceBrowser->setThaiFont(thaiFont);
    m_serviceBrowser->addThaiService(thaiLabel);
}
```

**Deliverables:**
- ✅ ETISnoop data pipeline connected to Qt GUI
- ✅ Thai character encoding and display functional
- ✅ Real-time service information updates working
- ✅ Error logging and alert integration operational

---

## 📋 **MIGRATION STRATEGY**

### **Phase 1: Placeholder Replacement**
**Timeline:** Day 4-7 | **Responsible:** Standards Compliance Agent + UI/UX Agent

**Files to Migrate:**
1. **src/core/eti_processor.cpp** → Replace with ETISnoop wrapper calls
2. **src/core/etsi_compliance.cpp** → Use ETISnoop validation features
3. **src/gui/analyzer_widget.cpp** → Connect to ETISnoop data streams
4. **src/gui/service_browser.cpp** → Display ETISnoop service information

**Migration Pattern:**
```cpp
// BEFORE (Placeholder)
bool ETIProcessor::processFrame(const QByteArray &data) {
    // TODO: Implement ETI frame processing
    return false;
}

// AFTER (ETISnoop Integration)
bool ETIProcessor::processFrame(const QByteArray &data) {
    return m_etisnoopWrapper->processETIFrame(data);
}
```

### **Phase 2: Feature Enhancement**
**Timeline:** Week 2 | **Responsible:** All Agents

**Enhanced Features via ETISnoop:**
- **Real-time Processing:** Use ETISnoop's optimized frame processing
- **Thai Language Support:** Leverage pre-integrated Thai DAB features
- **ETSI Compliance:** Utilize ETISnoop's proven standards implementation
- **Error Detection:** Use ETISnoop's comprehensive validation system

### **Phase 3: Testing & Validation**
**Timeline:** Week 2-3 | **Responsible:** Tester Agent + Standards Compliance Agent

**Testing Requirements:**
1. **Unit Tests:** Verify ETISnoop wrapper functionality
2. **Integration Tests:** Test Qt GUI with ETISnoop data pipeline
3. **Thai DAB Tests:** Validate Thai character encoding and service display
4. **Performance Tests:** Ensure >900 frames/second processing capability
5. **Compliance Tests:** Verify ETSI standards implementation

---

## 🎯 **SUCCESS CRITERIA**

### **Phase 0 Completion Criteria:**
- [ ] **ETISnoop submodule integrated** - Repository cloned and configured
- [ ] **Build system operational** - CMake compiles ETISnoop libraries successfully
- [ ] **C++ wrapper functional** - ETISnoop wrapper classes operational
- [ ] **Qt integration working** - GUI receives and displays ETISnoop data
- [ ] **Thai DAB support validated** - Thai characters display correctly
- [ ] **Basic file processing** - Can open and process real ETI files

### **Migration Success Criteria:**
- [ ] **Placeholder code removed** - All TODO implementations replaced
- [ ] **Real functionality operational** - Actual ETI processing working
- [ ] **Performance targets met** - >900 frames/second processing achieved
- [ ] **Thai standards compliance** - Thai DAB features fully operational
- [ ] **Error-free builds** - Application compiles without warnings
- [ ] **GUI responsiveness** - 60 FPS updates with real-time data

---

## 📊 **RISK ASSESSMENT**

### **Technical Risks:**
- **Low Risk:** ETISnoop is proven, mature software
- **Build Complexity:** CMake integration may require dependency resolution
- **Thai Support:** Character encoding may need Qt localization setup
- **Performance:** Need to verify ETISnoop meets our performance targets

### **Mitigation Strategies:**
- **Incremental Integration:** Add ETISnoop functionality step by step
- **Parallel Development:** Keep placeholder code until ETISnoop integration complete
- **Testing Focus:** Extensive testing with Thai DAB streams
- **Documentation:** Document all integration steps for team coordination

---

## 🏆 **EXPECTED OUTCOMES**

### **Immediate Benefits:**
- **Proven ETI Processing:** Replace placeholder code with working implementation
- **Thai Market Ready:** Immediate support for Thai DAB broadcasting standards
- **ETSI Compliance:** Complete standards implementation from day one
- **Development Acceleration:** Focus on GUI and features instead of ETI parsing

### **Long-term Benefits:**
- **Professional Quality:** ETISnoop provides broadcast-grade ETI processing
- **Community Support:** Access to ongoing ETISnoop development and fixes
- **Market Differentiation:** Thai DAB support provides competitive advantage
- **Reduced Risk:** Building on proven foundation instead of developing from scratch

---

## 📋 **AGENT COORDINATION PLAN**

### **Agent Responsibilities:**
1. **PM Agent:** Overall coordination, timeline enforcement, integration oversight
2. **Build Manager Agent:** CMake integration, submodule configuration, dependency resolution
3. **Standards Compliance Agent:** C++ wrapper development, ETSI compliance validation
4. **UI/UX Agent:** Qt GUI integration, Thai character rendering, real-time displays
5. **Network/Stream Agent:** Real-time streaming integration with ETISnoop backend
6. **Tester Agent:** TDD framework integration, ETISnoop testing, validation

### **Coordination Workflow:**
1. **Day 1:** PM Agent initiates ETISnoop integration, coordinates with Build Manager
2. **Day 2-3:** Build Manager configures submodule and CMake integration
3. **Day 4-6:** Standards Compliance Agent develops C++ wrapper
4. **Day 7-9:** UI/UX Agent integrates Qt GUI with ETISnoop data pipeline
5. **Day 10-12:** Network/Stream Agent adds real-time processing capabilities
6. **Day 13-14:** Tester Agent validates integration and performance

### **Communication Protocol:**
- **Daily Standups:** Progress updates and blocker identification
- **Integration Testing:** Continuous validation of ETISnoop integration
- **Thai DAB Validation:** Regular testing with Thai language content
- **Performance Monitoring:** Continuous assessment of processing capabilities

---

**Status:** Ready for immediate implementation - ETISnoop integration is now the highest priority task for all agents.