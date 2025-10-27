/**
 * Test: Load Bangkok ETI file and trace FIC data flow
 */

#include <QCoreApplication>
#include <QDebug>
#include <QFile>
#include <memory>
#include "../src/core/enhanced_eti_processor_qt.h"
#include "../src/core/advanced_fig_analyzer.h"

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);
    
    qDebug() << "=== Bangkok ETI Loading Test ===";
    
    QString testFile = "eti/bkk_20062022_141637.eti";
    
    // Check file exists
    QFile file(testFile);
    if (!file.exists()) {
        qCritical() << "File not found:" << testFile;
        return 1;
    }
    
    qDebug() << "File:" << testFile;
    qDebug() << "Size:" << file.size() << "bytes";
    qDebug() << "Frames:" << (file.size() / 6144);
    
    // Create components
    auto processor = std::make_unique<EnhancedETIProcessorQt>();
    auto analyzer = std::make_unique<AdvancedFIGAnalyzer>();
    
    int frameCount = 0;
    int ficFrameCount = 0;
    int serviceUpdateCount = 0;
    
    // Connect to trace data flow
    QObject::connect(processor.get(), &EnhancedETIProcessorQt::frameProcessed,
                     [&](ProcessedFrame frame) {
                         frameCount++;
                         
                         if (frameCount <= 5 || frameCount % 100 == 0) {
                             qDebug() << "\n[Frame" << frame.frame_number << "]";
                             qDebug() << "  FIC data size:" << frame.fic_data.size() << "bytes";
                             qDebug() << "  MSC data size:" << frame.msc_data.size() << "bytes";
                             
                             if (!frame.fic_data.isEmpty()) {
                                 qDebug() << "  FIC hex:" << frame.fic_data.left(16).toHex();
                             }
                         }
                         
                         // THIS IS THE CRITICAL PART - pass FIC to analyzer
                         if (!frame.fic_data.isEmpty()) {
                             ficFrameCount++;
                             analyzer->analyzeFICData(frame.fic_data);
                         }
                     });
    
    QObject::connect(analyzer.get(), &AdvancedFIGAnalyzer::servicesUpdated,
                     [&]() {
                         serviceUpdateCount++;
                         int count = analyzer->getServiceCount();
                         qDebug() << "\n***** servicesUpdated signal #" << serviceUpdateCount << "*****";
                         qDebug() << "Service count:" << count;
                         
                         QList<ServiceInfo> services = analyzer->getDiscoveredServices();
                         for (const auto& service : services) {
                             qDebug() << "  -" << service.label 
                                      << "(0x" << QString::number(service.serviceId, 16) << ")";
                         }
                     });
    
    QObject::connect(analyzer.get(), &AdvancedFIGAnalyzer::ensembleInfoUpdated,
                     [&](const EnsembleInfo& info) {
                         qDebug() << "\n***** ensembleInfoUpdated signal *****";
                         qDebug() << "Ensemble:" << info.ensembleLabel;
                         qDebug() << "EID: 0x" << QString::number(info.ensembleId, 16);
                     });
    
    // Process file
    qDebug() << "\nProcessing file...";
    bool result = processor->processETIFile(testFile);
    
    if (!result) {
        qCritical() << "Failed to process ETI file";
        return 1;
    }
    
    // Wait for all signals
    QCoreApplication::processEvents();
    
    qDebug() << "\n=== Processing Complete ===";
    qDebug() << "Total frames processed:" << frameCount;
    qDebug() << "Frames with FIC data:" << ficFrameCount;
    qDebug() << "servicesUpdated signals:" << serviceUpdateCount;
    qDebug() << "Final service count:" << analyzer->getServiceCount();
    
    // Print final services
    QList<ServiceInfo> finalServices = analyzer->getDiscoveredServices();
    qDebug() << "\nDiscovered Services:";
    for (const auto& service : finalServices) {
        qDebug() << "  -" << service.label 
                 << "SID: 0x" << QString::number(service.serviceId, 16)
                 << "Type:" << service.type;
    }
    
    if (finalServices.isEmpty()) {
        qCritical() << "\n❌ NO SERVICES DISCOVERED - THIS IS THE PROBLEM";
        qDebug() << "Possible reasons:";
        qDebug() << "1. FIC data is empty in all frames";
        qDebug() << "2. FIG analyzer is not parsing FIC correctly";
        qDebug() << "3. Signal/slot connection broken";
        return 1;
    } else {
        qDebug() << "\n✅ Services discovered successfully";
        return 0;
    }
}
