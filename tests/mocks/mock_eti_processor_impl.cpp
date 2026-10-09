/**
 * @file mock_eti_processor_impl.cpp
 * @brief Mock ETI processor implementation for test build compatibility
 * 
 * Provides minimal implementations to prevent test build failures while
 * avoiding actual ETI processing that causes performance issues.
 */

#include "mock_eti_processor.h"
#include <QDebug>
#include <QTimer>
#include <QApplication>

// Mock implementations for TDD Factory
std::unique_ptr<MockEtiProcessor> TddMockFactory::createSuccessfulProcessor() {
    auto processor = std::make_unique<MockEtiProcessor>();
    
    // Configure for successful processing
    ON_CALL(*processor, processFrame(testing::_))
        .WillByDefault(testing::Return(ProcessResult{true, "", 1, 3}));
    
    ON_CALL(*processor, isValidFrame(testing::_))
        .WillByDefault(testing::Return(true));
    
    ON_CALL(*processor, getServices())
        .WillByDefault(testing::Return(std::vector<ServiceInfo>()));
    
    ON_CALL(*processor, getFrameCount())
        .WillByDefault(testing::Return(100));
    
    ON_CALL(*processor, hasErrors())
        .WillByDefault(testing::Return(false));
    
    return processor;
}

std::unique_ptr<MockEtiProcessor> TddMockFactory::createFailingProcessor() {
    auto processor = std::make_unique<MockEtiProcessor>();
    
    // Configure for failed processing
    ON_CALL(*processor, processFrame(testing::_))
        .WillByDefault(testing::Return(ProcessResult{false, "Test failure", 0, 0}));
    
    ON_CALL(*processor, isValidFrame(testing::_))
        .WillByDefault(testing::Return(false));
    
    ON_CALL(*processor, hasErrors())
        .WillByDefault(testing::Return(true));
    
    ON_CALL(*processor, getErrorMessages())
        .WillByDefault(testing::Return(std::vector<std::string>{"Test error"}));
    
    return processor;
}

std::unique_ptr<MockEnsembleManager> TddMockFactory::createMultiServiceEnsemble() {
    auto manager = std::make_unique<MockEnsembleManager>();
    
    // Configure with multiple test services
    std::vector<ServiceInfo> testServices;
    for (int i = 0; i < 3; ++i) {
        ServiceInfo service;
        service.serviceId = 0xD100 + i;
        service.label = QString("Test Service %1").arg(i + 1).toStdString();
        testServices.push_back(service);
    }
    
    ON_CALL(*manager, getAllServices())
        .WillByDefault(testing::Return(testServices));
    
    ON_CALL(*manager, getServiceCount())
        .WillByDefault(testing::Return(testServices.size()));
    
    ON_CALL(*manager, hasService(testing::_))
        .WillByDefault(testing::Return(true));
    
    ON_CALL(*manager, isValid())
        .WillByDefault(testing::Return(true));
    
    return manager;
}

std::unique_ptr<MockAudioDecoder> TddMockFactory::createWorkingAudioDecoder() {
    auto decoder = std::make_unique<MockAudioDecoder>();
    
    // Configure for successful audio decoding
    ON_CALL(*decoder, decode(testing::_))
        .WillByDefault(testing::Return(true));
    
    ON_CALL(*decoder, supportsFormat(testing::_))
        .WillByDefault(testing::Return(true));
    
    ON_CALL(*decoder, getSampleRate())
        .WillByDefault(testing::Return(48000));
    
    ON_CALL(*decoder, getBitRate())
        .WillByDefault(testing::Return(128));
    
    ON_CALL(*decoder, getChannels())
        .WillByDefault(testing::Return(2));
    
    ON_CALL(*decoder, getFormat())
        .WillByDefault(testing::Return("DAB+"));
    
    ON_CALL(*decoder, isConfigured())
        .WillByDefault(testing::Return(true));
    
    return decoder;
}

std::tuple<
    std::unique_ptr<MockEtiProcessor>,
    std::unique_ptr<MockFicDecoder>,
    std::unique_ptr<MockDabDecoder>,
    std::unique_ptr<MockEnsembleManager>
> TddMockFactory::createCompletePipeline() {
    
    auto processor = createSuccessfulProcessor();
    auto ficDecoder = std::make_unique<MockFicDecoder>();
    auto dabDecoder = std::make_unique<MockDabDecoder>();
    auto ensembleManager = createMultiServiceEnsemble();
    
    // Configure FIC decoder
    ON_CALL(*ficDecoder, decodeFic(testing::_))
        .WillByDefault(testing::Return(true));
    
    ON_CALL(*ficDecoder, isValidFicBlock(testing::_))
        .WillByDefault(testing::Return(true));
    
    ON_CALL(*ficDecoder, hasEnsembleInfo())
        .WillByDefault(testing::Return(true));
    
    ON_CALL(*ficDecoder, getEnsembleLabel())
        .WillByDefault(testing::Return("Test Ensemble"));
    
    // Configure DAB decoder
    ON_CALL(*dabDecoder, decode(testing::_))
        .WillByDefault(testing::Return(true));
    
    ON_CALL(*dabDecoder, isValidAudioFrame(testing::_))
        .WillByDefault(testing::Return(true));
    
    ON_CALL(*dabDecoder, getSampleRate())
        .WillByDefault(testing::Return(48000));
    
    ON_CALL(*dabDecoder, getChannelCount())
        .WillByDefault(testing::Return(2));
    
    ON_CALL(*dabDecoder, getAudioType())
        .WillByDefault(testing::Return("DAB+"));
    
    return std::make_tuple(
        std::move(processor),
        std::move(ficDecoder),
        std::move(dabDecoder),
        std::move(ensembleManager)
    );
}

// Test utilities for performance optimization
namespace TestUtils {

bool isTestModeEnabled() {
    return qgetenv("ETI_TEST_MODE") == "1" || 
           qgetenv("QT_QPA_PLATFORM") == "offscreen";
}

void configureOptimalTestEnvironment() {
    // Set environment variables for optimal test performance
    qputenv("ETI_TEST_MODE", "1");
    qputenv("ETI_MINIMAL_PROCESSING", "1");
    qputenv("ETI_DISABLE_ANALYSIS", "1");
    qputenv("QT_LOGGING_RULES", "*.debug=false");
    
    // Configure Qt application for testing
    if (QApplication::instance()) {
        QApplication::instance()->setProperty("test_mode", true);
        QApplication::instance()->setProperty("test_frame_count", 0);
    }
    
    qDebug() << "Test environment configured for optimal performance";
}

void resetTestEnvironment() {
    if (QApplication::instance()) {
        QApplication::instance()->setProperty("test_mode", false);
        QApplication::instance()->setProperty("test_frame_count", 0);
    }
    
    qDebug() << "Test environment reset";
}

} // namespace TestUtils