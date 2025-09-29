#include <QApplication>
#include <QElapsedTimer>
#include <QFileInfo>
#include <iostream>
#include <memory>
#include "src/core/enhanced_eti_processor_qt.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    
    std::cout << "==============================================\n";
    std::cout << "ACCURATE PERFORMANCE MEASUREMENT TEST\n";
    std::cout << "==============================================\n";
    
    QString testFile = "/home/seksan/workspace/streamdab-analyzer/test_stream.eti";
    QFileInfo fileInfo(testFile);
    
    if (!fileInfo.exists()) {
        std::cout << "❌ Test file not found\n";
        return 1;
    }
    
    std::cout << "📁 File: " << testFile.toStdString() << "\n";
    std::cout << "📊 Size: " << fileInfo.size() << " bytes\n";
    std::cout << "📏 Expected frames: " << (fileInfo.size() / 6144) << "\n";
    
    // Test multiple runs for accuracy
    const int numRuns = 5;
    std::vector<double> fpsResults;
    
    for (int run = 1; run <= numRuns; ++run) {
        std::cout << "\n--- RUN " << run << " ---\n";
        
        auto processor = std::make_unique<EnhancedETIProcessorQt>();
        
        // High-precision timing
        QElapsedTimer timer;
        timer.start();
        
        bool success = processor->processETIFile(testFile);
        
        qint64 elapsedMs = timer.elapsed();
        qint64 elapsedMicroSec = timer.nsecsElapsed() / 1000;
        
        if (success) {
            int frameCount = processor->getFrameCount();
            double fps = (elapsedMs > 0) ? (1000.0 * frameCount / elapsedMs) : 0.0;
            double microFps = (elapsedMicroSec > 0) ? (1000000.0 * frameCount / elapsedMicroSec) : 0.0;
            
            std::cout << "✅ Processing complete\n";
            std::cout << "   Frames: " << frameCount << "\n";
            std::cout << "   Time (ms): " << elapsedMs << "\n";
            std::cout << "   Time (μs): " << elapsedMicroSec << "\n";
            std::cout << "   FPS (ms precision): " << fps << "\n";
            std::cout << "   FPS (μs precision): " << microFps << "\n";
            
            fpsResults.push_back(microFps);
        } else {
            std::cout << "❌ Processing failed\n";
        }
    }
    
    // Calculate average performance
    if (!fpsResults.empty()) {
        double totalFps = 0.0;
        double maxFps = 0.0;
        double minFps = fpsResults[0];
        
        for (double fps : fpsResults) {
            totalFps += fps;
            maxFps = std::max(maxFps, fps);
            minFps = std::min(minFps, fps);
        }
        
        double avgFps = totalFps / fpsResults.size();
        
        std::cout << "\n==============================================\n";
        std::cout << "PERFORMANCE SUMMARY\n";
        std::cout << "==============================================\n";
        std::cout << "Average FPS: " << avgFps << "\n";
        std::cout << "Maximum FPS: " << maxFps << "\n";
        std::cout << "Minimum FPS: " << minFps << "\n";
        
        std::cout << "\n🎯 TARGET VALIDATION:\n";
        if (avgFps > 900.0) {
            std::cout << "✅ PERFORMANCE TARGET MET: " << avgFps << " fps > 900 fps\n";
        } else {
            std::cout << "⚠️  Performance below target: " << avgFps << " fps < 900 fps\n";
        }
        
        // Check for the claimed 135,162 fps
        if (avgFps > 100000.0) {
            std::cout << "🚀 EXCEPTIONAL PERFORMANCE: " << avgFps << " fps (>100k fps)\n";
        }
        
        std::cout << "==============================================\n";
    }
    
    return 0;
}