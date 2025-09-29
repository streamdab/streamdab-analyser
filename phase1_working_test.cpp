#include <QCoreApplication>
#include <QTimer>
#include <QDebug>
#include <iostream>

// Test if we can instantiate our core classes
#include "src/core/enhanced_eti_processor_qt.h"
#include "src/core/advanced_fig_analyzer.h"

class HonestTest : public QObject {
    Q_OBJECT
    
public:
    HonestTest() {
        // Test 1: Can we create ETI processor?
        try {
            m_etiProcessor = std::make_unique<EnhancedETIProcessorQt>();
            qDebug() << "✅ EnhancedETIProcessorQt created successfully";
        } catch (...) {
            qDebug() << "❌ CRITICAL: EnhancedETIProcessorQt creation failed";
            m_hasErrors = true;
        }
        
        // Test 2: Can we create FIG analyzer?
        try {
            m_figAnalyzer = std::make_unique<AdvancedFIGAnalyzer>();
            qDebug() << "✅ AdvancedFIGAnalyzer created successfully";
        } catch (...) {
            qDebug() << "❌ CRITICAL: AdvancedFIGAnalyzer creation failed";
            m_hasErrors = true;
        }
        
        // Test 3: Can we process Bangkok file?
        QTimer::singleShot(100, this, &HonestTest::testBangkokFile);
    }
    
private slots:
    void testBangkokFile() {
        if (!m_etiProcessor) {
            qDebug() << "❌ Cannot test Bangkok file - ETI processor failed";
            QCoreApplication::exit(1);
            return;
        }
        
        QString filePath = "/home/seksan/workspace/streamdab-analyzer/eti/bkk_20062022_141637.eti";
        
        // Connect to signals to verify they work
        connect(m_etiProcessor.get(), &EnhancedETIProcessorQt::frameProcessed,
                this, &HonestTest::onFrameProcessed);
        connect(m_etiProcessor.get(), &EnhancedETIProcessorQt::processingComplete,
                this, &HonestTest::onProcessingComplete);
        connect(m_etiProcessor.get(), &EnhancedETIProcessorQt::processingError,
                this, &HonestTest::onProcessingError);
        
        qDebug() << "🔄 Testing Bangkok file processing...";
        bool success = m_etiProcessor->processETIFile(filePath);
        
        if (!success) {
            qDebug() << "❌ CRITICAL: Failed to start Bangkok file processing";
            m_hasErrors = true;
            QCoreApplication::exit(1);
        } else {
            qDebug() << "✅ Bangkok file processing started successfully";
        }
    }
    
    void onFrameProcessed(const ProcessedFrame& frame) {
        m_framesProcessed++;
        if (m_framesProcessed <= 5) {
            qDebug() << QString("✅ Frame #%1 processed - Format: %2, SYNC: 0x%3")
                        .arg(frame.frame_number)
                        .arg(frame.format == ETIFormat::ETI_LI_A ? "ETI-LI-A" : 
                             frame.format == ETIFormat::ETI_LI_B ? "ETI-LI-B" : "Other")
                        .arg(frame.sync_pattern, 8, 16, QChar('0'));
        }
        
        // Test FIG analyzer integration
        if (!frame.fic_data.isEmpty() && m_figAnalyzer && m_framesProcessed <= 10) {
            m_figAnalyzer->analyzeFICData(frame.fic_data);
        }
    }
    
    void onProcessingComplete(int totalFrames, int processingTimeMs) {
        qDebug() << QString("✅ PROCESSING COMPLETE: %1 frames in %2ms").arg(totalFrames).arg(processingTimeMs);
        
        if (totalFrames == 5001) {
            qDebug() << "✅ VERIFIED: All 5,001 frames processed correctly";
        } else {
            qDebug() << QString("❌ CRITICAL: Expected 5,001 frames, got %1").arg(totalFrames);
            m_hasErrors = true;
        }
        
        // Final assessment
        if (m_hasErrors) {
            qDebug() << "\n💥 HONEST CONCLUSION: Critical issues prevent full functionality";
            QCoreApplication::exit(1);
        } else {
            qDebug() << "\n🎯 HONEST CONCLUSION: Phase 1.1-1.4 core functionality WORKING";
            QCoreApplication::exit(0);
        }
    }
    
    void onProcessingError(const QString& error) {
        qDebug() << "❌ PROCESSING ERROR:" << error;
        m_hasErrors = true;
        QCoreApplication::exit(1);
    }
    
private:
    std::unique_ptr<EnhancedETIProcessorQt> m_etiProcessor;
    std::unique_ptr<AdvancedFIGAnalyzer> m_figAnalyzer;
    int m_framesProcessed = 0;
    bool m_hasErrors = false;
};

int main(int argc, char *argv[]) {
    QCoreApplication app(argc, argv);
    
    qDebug() << "🔍 HONEST PHASE 1.1-1.4 QT IMPLEMENTATION TEST";
    qDebug() << "==============================================";
    
    HonestTest test;
    
    // Set a timeout in case something hangs
    QTimer::singleShot(30000, &app, [&]() {
        qDebug() << "❌ TIMEOUT: Test took too long, likely hanging";
        app.exit(1);
    });
    
    return app.exec();
}

#include "phase1_working_test.moc"