#include <iostream>
#include <QCoreApplication>
#include "src/core/enhanced_eti_processor_qt.h"

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);
    
    std::cout << "==============================================\n";
    std::cout << "Phase 1.4 Immediate Processing Test\n";
    std::cout << "Testing: Bangkok ETI All Frames Processing\n";
    std::cout << "==============================================\n";
    
    // Test EnhancedETIProcessorQt instantiation
    auto processor = std::make_unique<EnhancedETIProcessorQt>();
    std::cout << "✓ EnhancedETIProcessorQt: Created successfully\n";
    
    // Test Bangkok ETI file processing
    const QString etiFilePath = "eti/bkk_20062022_141637.eti";
    std::cout << "📁 Testing file: " << etiFilePath.toStdString() << "\n";
    
    // Connect signals to track processing
    QObject::connect(processor.get(), &EnhancedETIProcessorQt::processingComplete,
                     [](int totalFrames, int processingTimeMs) {
        std::cout << "🎉 Bangkok ETI Processing: COMPLETE!\n";
        std::cout << "   Total frames: " << totalFrames << "\n";
        std::cout << "   Processing time: " << processingTimeMs << " ms\n";
        std::cout << "   Processing rate: " << (1000.0 * totalFrames / processingTimeMs) << " fps\n";
        
        if (totalFrames == 5001) {
            std::cout << "✅ PHASE 1.4 VALIDATION: SUCCESS - All 5,001 frames processed!\n";
        } else {
            std::cout << "⚠️  Frame count mismatch (expected 5001, got " << totalFrames << ")\n";
        }
        
        QCoreApplication::quit();
    });
    
    QObject::connect(processor.get(), &EnhancedETIProcessorQt::processingError,
                     [](const QString& error) {
        std::cout << "❌ Processing Error: " << error.toStdString() << "\n";
        QCoreApplication::quit();
    });
    
    if (processor->processETIFile(etiFilePath)) {
        std::cout << "✓ File loading initiated...\n";
    } else {
        std::cout << "❌ Failed to load file!\n";
        return 1;
    }
    
    // Run event loop for signal processing
    return app.exec();
}