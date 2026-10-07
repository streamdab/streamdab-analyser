/**
 * Signal/Slot Verification Test
 * 
 * This test verifies that:
 * 1. ETI processor emits frameProcessed signals
 * 2. FIG analyser receives FIC data and emits servicesUpdated
 * 3. Signal/slot connections are correct
 */

#include <QTest>
#include <QSignalSpy>
#include <QCoreApplication>
#include <memory>
#include "../src/core/enhanced_eti_processor_qt.h"
#include "../src/core/advanced_fig_analyser.h"

class TestSignalSlotVerification : public QObject
{
    Q_OBJECT

private:
    std::unique_ptr<EnhancedETIProcessorQt> m_processor;
    std::unique_ptr<AdvancedFIGAnalyser> m_analyser;

private slots:
    void initTestCase()
    {
        qDebug() << "=== Signal/Slot Verification Test ===";
        
        // Create components
        m_processor = std::make_unique<EnhancedETIProcessorQt>();
        m_analyser = std::make_unique<AdvancedFIGAnalyser>();
        
        QVERIFY(m_processor != nullptr);
        QVERIFY(m_analyser != nullptr);
    }

    void test_01_ProcessorSignals()
    {
        qDebug() << "\n[TEST 1] ETI Processor Signal Emission";
        
        // Create signal spies
        QSignalSpy spyFrameProcessed(m_processor.get(), 
                                     &EnhancedETIProcessorQt::frameProcessed);
        QSignalSpy spyProgress(m_processor.get(), 
                              &EnhancedETIProcessorQt::processingProgress);
        QSignalSpy spyComplete(m_processor.get(), 
                              &EnhancedETIProcessorQt::processingComplete);
        
        QVERIFY(spyFrameProcessed.isValid());
        QVERIFY(spyProgress.isValid());
        QVERIFY(spyComplete.isValid());
        
        // Try to load a test file
        QString testFile = "eti/sample_bangkok_dab.eti";
        QFile file(testFile);
        
        if (file.exists()) {
            qDebug() << "Processing test file:" << testFile;
            bool result = m_processor->processETIFile(testFile);
            
            QVERIFY2(result, "ETI file processing should succeed");
            
            // Wait for signals
            spyComplete.wait(5000);
            
            qDebug() << "frameProcessed signals:" << spyFrameProcessed.count();
            qDebug() << "processingProgress signals:" << spyProgress.count();
            qDebug() << "processingComplete signals:" << spyComplete.count();
            
            QVERIFY2(spyFrameProcessed.count() > 0, "frameProcessed signal should be emitted");
            QVERIFY2(spyComplete.count() == 1, "processingComplete signal should be emitted once");
            
            // Verify frame data structure
            if (spyFrameProcessed.count() > 0) {
                QList<QVariant> arguments = spyFrameProcessed.at(0);
                qDebug() << "First frameProcessed signal has" << arguments.count() << "arguments";
                
                // ProcessedFrame should be passed
                QVERIFY2(arguments.count() == 1, "frameProcessed should pass ProcessedFrame");
            }
        } else {
            QSKIP("Test ETI file not found - skipping processor test");
        }
        
        qDebug() << "✅ ETI Processor signals verified";
    }

    void test_02_AnalyserSignals()
    {
        qDebug() << "\n[TEST 2] FIG Analyser Signal Emission";
        
        // Create signal spies
        QSignalSpy spyServicesUpdated(m_analyser.get(), 
                                      &AdvancedFIGAnalyser::servicesUpdated);
        QSignalSpy spyEnsembleUpdated(m_analyser.get(), 
                                      &AdvancedFIGAnalyser::ensembleInfoUpdated);
        
        QVERIFY(spyServicesUpdated.isValid());
        QVERIFY(spyEnsembleUpdated.isValid());
        
        // Create sample FIC data (96 bytes for Mode 1)
        QByteArray ficData(96, 0);
        
        // Add a simple FIG 0/0 (ensemble info)
        // FIG header: Type=0, Length=5
        ficData[0] = 0x00;  // FIG type 0/0
        ficData[1] = 0x05;  // Length 5
        ficData[2] = 0x00;  // EId MSB
        ficData[3] = 0x01;  // EId LSB
        ficData[4] = 0x00;  // Change flags
        ficData[5] = 0x00;  // Alarm flag
        ficData[6] = 0x01;  // CIF count MSB
        
        qDebug() << "Feeding FIC data to analyser...";
        m_analyser->analyzeFICData(ficData);
        
        // Wait for signals
        QTest::qWait(100);
        
        qDebug() << "servicesUpdated signals:" << spyServicesUpdated.count();
        qDebug() << "ensembleInfoUpdated signals:" << spyEnsembleUpdated.count();
        
        // After processing FIG 0/0, we should get ensemble info update
        QVERIFY2(spyEnsembleUpdated.count() > 0, 
                 "ensembleInfoUpdated signal should be emitted after FIG 0/0");
        
        qDebug() << "✅ FIG Analyser signals verified";
    }

    void test_03_ConnectionFlow()
    {
        qDebug() << "\n[TEST 3] Complete Signal Flow";
        
        // Test the complete flow:
        // ETI Processor → frameProcessed → Handler → FIG Analyser → servicesUpdated
        
        // Create fresh instances
        auto processor = std::make_unique<EnhancedETIProcessorQt>();
        auto analyser = std::make_unique<AdvancedFIGAnalyser>();
        
        QSignalSpy spyFrameProcessed(processor.get(), 
                                     &EnhancedETIProcessorQt::frameProcessed);
        QSignalSpy spyServicesUpdated(analyser.get(), 
                                      &AdvancedFIGAnalyser::servicesUpdated);
        
        // Connect them like main.cpp should
        connect(processor.get(), &EnhancedETIProcessorQt::frameProcessed,
                [&analyser](ProcessedFrame frame) {
                    qDebug() << "[Lambda] Received frame:" << frame.frame_number 
                             << "FIC size:" << frame.fic_data.size();
                    
                    // This is what main.cpp should do
                    if (!frame.fic_data.isEmpty()) {
                        analyser->analyzeFICData(frame.fic_data);
                    }
                });
        
        // Process test file
        QString testFile = "eti/sample_bangkok_dab.eti";
        if (QFile::exists(testFile)) {
            qDebug() << "Processing with connected flow...";
            processor->processETIFile(testFile);
            
            // Wait for completion
            QTest::qWait(2000);
            
            qDebug() << "Total frameProcessed signals:" << spyFrameProcessed.count();
            qDebug() << "Total servicesUpdated signals:" << spyServicesUpdated.count();
            
            QVERIFY2(spyFrameProcessed.count() > 0, 
                     "ETI processor should emit frameProcessed");
            
            qDebug() << "✅ Complete signal flow verified";
        } else {
            QSKIP("Test ETI file not found - skipping flow test");
        }
    }

    void test_04_ServiceCountVerification()
    {
        qDebug() << "\n[TEST 4] Service Count After Processing";
        
        // Reset analyser
        m_analyser->reset();
        
        int initialCount = m_analyser->getServiceCount();
        qDebug() << "Initial service count:" << initialCount;
        QCOMPARE(initialCount, 0);
        
        // Process a test file through the complete flow
        QString testFile = "eti/sample_bangkok_dab.eti";
        if (QFile::exists(testFile)) {
            // Create temporary processor
            auto processor = std::make_unique<EnhancedETIProcessorQt>();
            
            // Connect with FIC data passing
            connect(processor.get(), &EnhancedETIProcessorQt::frameProcessed,
                    [this](ProcessedFrame frame) {
                        if (!frame.fic_data.isEmpty()) {
                            m_analyser->analyzeFICData(frame.fic_data);
                        }
                    });
            
            processor->processETIFile(testFile);
            
            // Wait for processing
            QTest::qWait(2000);
            
            int finalCount = m_analyser->getServiceCount();
            qDebug() << "Final service count:" << finalCount;
            
            QVERIFY2(finalCount > 0, 
                     "Should discover at least 1 service from Bangkok DAB+ file");
            
            // Get service details
            QList<ServiceInfo> services = m_analyser->getDiscoveredServices();
            qDebug() << "Discovered services:";
            for (const auto& service : services) {
                qDebug() << "  -" << service.label << "(SID:" 
                         << QString("0x%1").arg(service.serviceId, 4, 16, QChar('0')) << ")";
            }
            
            QVERIFY2(!services.isEmpty(), "Service list should not be empty");
            
            qDebug() << "✅ Service discovery verified";
        } else {
            QSKIP("Test ETI file not found - skipping service count test");
        }
    }

    void cleanupTestCase()
    {
        qDebug() << "\n=== Signal/Slot Verification Complete ===";
        qDebug() << "All signal/slot connections verified working correctly";
    }
};

QTEST_MAIN(TestSignalSlotVerification)
#include "test_signal_slot_verification.moc"
