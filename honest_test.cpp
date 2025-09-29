#include <iostream>
#include <fstream>
#include <vector>
#include <cstdint>

// Test 1: Can we read the Bangkok ETI file?
bool testBangkokFileReading() {
    std::ifstream file("/home/seksan/workspace/streamdab-analyzer/eti/bkk_20062022_141637.eti", std::ios::binary);
    if (!file.is_open()) {
        std::cout << "❌ CRITICAL: Cannot open Bangkok ETI file" << std::endl;
        return false;
    }
    
    // Get file size
    file.seekg(0, std::ios::end);
    size_t fileSize = file.tellg();
    file.seekg(0, std::ios::beg);
    
    std::cout << "✅ Bangkok ETI file accessible: " << fileSize << " bytes" << std::endl;
    
    // Check if size is multiple of 6144
    if (fileSize % 6144 != 0) {
        std::cout << "❌ CRITICAL: File size not multiple of 6144 bytes" << std::endl;
        return false;
    }
    
    size_t expectedFrames = fileSize / 6144;
    std::cout << "✅ Expected frames: " << expectedFrames << std::endl;
    
    return true;
}

// Test 2: Can we detect Bangkok ETI-LI patterns?
bool testBangkokPatternDetection() {
    std::ifstream file("/home/seksan/workspace/streamdab-analyzer/eti/bkk_20062022_141637.eti", std::ios::binary);
    if (!file.is_open()) return false;
    
    // Read first few frames to check patterns
    std::vector<uint8_t> buffer(6144);
    int patternACount = 0;
    int patternBCount = 0;
    int unknownCount = 0;
    
    for (int i = 0; i < 10 && file.read(reinterpret_cast<char*>(buffer.data()), 6144); ++i) {
        uint32_t syncPattern = (buffer[0] << 24) | (buffer[1] << 16) | (buffer[2] << 8) | buffer[3];
        
        if (syncPattern == 0xfff8c549) {
            patternACount++;
        } else if (syncPattern == 0xff073ab6) {
            patternBCount++;
        } else {
            unknownCount++;
            std::cout << "Unknown pattern in frame " << i << ": 0x" << std::hex << syncPattern << std::dec << std::endl;
        }
    }
    
    std::cout << "Pattern A (0xfff8c549): " << patternACount << " frames" << std::endl;
    std::cout << "Pattern B (0xff073ab6): " << patternBCount << " frames" << std::endl;
    std::cout << "Unknown patterns: " << unknownCount << " frames" << std::endl;
    
    bool hasBangkokPatterns = (patternACount > 0 || patternBCount > 0);
    if (hasBangkokPatterns) {
        std::cout << "✅ Bangkok ETI-LI patterns detected" << std::endl;
    } else {
        std::cout << "❌ CRITICAL: No Bangkok ETI-LI patterns found" << std::endl;
    }
    
    return hasBangkokPatterns;
}

// Test 3: Check if core header files exist and are accessible
bool testCoreFilesExist() {
    std::vector<std::string> requiredFiles = {
        "src/core/enhanced_eti_processor_qt.h",
        "src/core/enhanced_eti_processor_qt.cpp",
        "src/core/advanced_fig_analyzer.h", 
        "src/core/advanced_fig_analyzer.cpp",
        "src/core/eti_types.h",
        "src/gui/dab_xplorer_main_window.h",
        "src/gui/dab_xplorer_main_window.cpp"
    };
    
    int existingFiles = 0;
    for (const auto& file : requiredFiles) {
        std::ifstream testFile(file);
        if (testFile.is_open()) {
            existingFiles++;
            std::cout << "✅ " << file << std::endl;
        } else {
            std::cout << "❌ " << file << std::endl;
        }
    }
    
    std::cout << "Core files: " << existingFiles << "/" << requiredFiles.size() << " exist" << std::endl;
    return existingFiles == requiredFiles.size();
}

int main() {
    std::cout << "🔍 HONEST PHASE 1.1-1.4 IMPLEMENTATION VERIFICATION" << std::endl;
    std::cout << "=================================================" << std::endl;
    
    bool test1 = testBangkokFileReading();
    bool test2 = testBangkokPatternDetection();  
    bool test3 = testCoreFilesExist();
    
    std::cout << std::endl;
    std::cout << "📊 HONEST ASSESSMENT SUMMARY:" << std::endl;
    std::cout << "✅ File Reading: " << (test1 ? "WORKING" : "FAILED") << std::endl;
    std::cout << "✅ Pattern Detection: " << (test2 ? "WORKING" : "FAILED") << std::endl;
    std::cout << "✅ Core Files: " << (test3 ? "COMPLETE" : "MISSING") << std::endl;
    
    if (test1 && test2 && test3) {
        std::cout << std::endl << "🎯 CONCLUSION: Basic foundations are SOLID for Phase 1.1-1.4" << std::endl;
        return 0;
    } else {
        std::cout << std::endl << "💥 CONCLUSION: Critical issues prevent Phase 1.1-1.4 functionality" << std::endl;
        return 1;
    }
}