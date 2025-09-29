#include <iostream>
#include <QCoreApplication>
#include <QFile>
#include <QFileInfo>
#include <QDir>
#include "src/core/enhanced_eti_processor_qt.h"

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);
    
    std::cout << "==============================================\n";
    std::cout << "ETI FILE VALIDATION TEST\n";
    std::cout << "==============================================\n";
    
    // Test with absolute path
    QString basePath = "/home/seksan/workspace/streamdab-analyzer";
    QString testFile = basePath + "/test_stream.eti";
    QString etiFile = basePath + "/eti/bkk_20062022_141637.eti";
    
    std::cout << "Working directory: " << QDir::currentPath().toStdString() << "\n";
    std::cout << "Base path: " << basePath.toStdString() << "\n";
    
    // Check file existence
    QFileInfo testInfo(testFile);
    QFileInfo etiInfo(etiFile);
    
    std::cout << "test_stream.eti exists: " << (testInfo.exists() ? "YES" : "NO") << "\n";
    std::cout << "test_stream.eti size: " << testInfo.size() << " bytes\n";
    std::cout << "bangkok eti exists: " << (etiInfo.exists() ? "YES" : "NO") << "\n";
    std::cout << "bangkok eti size: " << etiInfo.size() << " bytes\n";
    
    // Test processor
    auto processor = std::make_unique<EnhancedETIProcessorQt>();
    
    // Test with test_stream.eti (should have 100 frames)
    std::cout << "\n--- Testing with test_stream.eti ---\n";
    if (processor->processETIFile(testFile)) {
        std::cout << "✓ test_stream.eti processing: SUCCESS\n";
        std::cout << "  Total frames: " << processor->getFrameCount() << "\n";
        std::cout << "  ETI-NI count: " << processor->getETINICount() << "\n";
        std::cout << "  ETI-LI-A count: " << processor->getETILIACount() << "\n";
        std::cout << "  ETI-LI-B count: " << processor->getETILIBCount() << "\n";
        
        if (processor->getFrameCount() == 100) {
            std::cout << "✅ Expected frame count VERIFIED (100 frames)\n";
        } else {
            std::cout << "⚠️  Frame count mismatch (expected 100, got " << processor->getFrameCount() << ")\n";
        }
    } else {
        std::cout << "❌ test_stream.eti processing: FAILED\n";
    }
    
    // Reset for next test
    processor.reset();
    processor = std::make_unique<EnhancedETIProcessorQt>();
    
    // Test with bangkok file
    std::cout << "\n--- Testing with bangkok file ---\n";
    if (processor->processETIFile(etiFile)) {
        std::cout << "✓ bangkok file processing: SUCCESS\n";
        std::cout << "  Total frames: " << processor->getFrameCount() << "\n";
        std::cout << "  ETI-NI count: " << processor->getETINICount() << "\n";
        std::cout << "  ETI-LI-A count: " << processor->getETILIACount() << "\n";
        std::cout << "  ETI-LI-B count: " << processor->getETILIBCount() << "\n";
    } else {
        std::cout << "❌ bangkok file processing: FAILED\n";
    }
    
    std::cout << "==============================================\n";
    
    return 0;
}