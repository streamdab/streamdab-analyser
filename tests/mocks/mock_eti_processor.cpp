#include "mock_eti_processor.h"
#include <memory>

using ::testing::Return;
using ::testing::_;
using ::testing::AtLeast;
using ::testing::InSequence;

/**
 * @file mock_eti_processor.cpp
 * @brief Implementation of TDD mock factory methods
 * 
 * These factory methods create pre-configured mocks for common TDD scenarios,
 * supporting Red-Green-Refactor workflow with realistic test data.
 */

std::unique_ptr<MockEtiProcessor> TddMockFactory::createSuccessfulProcessor() {
    auto mock = std::make_unique<MockEtiProcessor>();
    
    // Configure successful processing behavior
    ProcessResult successResult;
    successResult.success = true;
    successResult.framesProcessed = 1;
    successResult.servicesFound = 2;
    successResult.errorMessage = "";
    
    ON_CALL(*mock, process(_))
        .WillByDefault(Return(successResult));
    
    ON_CALL(*mock, processFrame(_))
        .WillByDefault(Return(successResult));
    
    ON_CALL(*mock, isValidFrame(_))
        .WillByDefault(Return(true));
    
    ON_CALL(*mock, isStreamValid(_))
        .WillByDefault(Return(true));
    
    ON_CALL(*mock, hasErrors())
        .WillByDefault(Return(false));
    
    ON_CALL(*mock, getFrameCount())
        .WillByDefault(Return(1000));
    
    // Configure service information
    std::vector<ServiceInfo> services;
    
    ServiceInfo service1;
    service1.serviceId = 0x1234;
    service1.label = "Test Service 1";
    service1.subChannelId = 1;
    service1.audioType = "DAB+";
    service1.bitRate = 128;
    service1.isActive = true;
    services.push_back(service1);
    
    ServiceInfo service2;
    service2.serviceId = 0x5678;
    service2.label = "Test Service 2";
    service2.subChannelId = 2;
    service2.audioType = "DAB";
    service2.bitRate = 192;
    service2.isActive = true;
    services.push_back(service2);
    
    ON_CALL(*mock, getServices())
        .WillByDefault(Return(services));
    
    // Configure ensemble information
    EnsembleInfo ensemble;
    ensemble.ensembleId = 0xABCD;
    ensemble.label = "Test Ensemble";
    ensemble.serviceCount = 2;
    ensemble.country = "GB";
    ensemble.isValid = true;
    
    ON_CALL(*mock, getEnsembleInfo())
        .WillByDefault(Return(ensemble));
    
    return mock;
}

std::unique_ptr<MockEtiProcessor> TddMockFactory::createFailingProcessor() {
    auto mock = std::make_unique<MockEtiProcessor>();
    
    // Configure failing processing behavior
    ProcessResult failResult;
    failResult.success = false;
    failResult.framesProcessed = 0;
    failResult.servicesFound = 0;
    failResult.errorMessage = "ETI processing failed: Invalid frame format";
    
    ON_CALL(*mock, process(_))
        .WillByDefault(Return(failResult));
    
    ON_CALL(*mock, processFrame(_))
        .WillByDefault(Return(failResult));
    
    ON_CALL(*mock, isValidFrame(_))
        .WillByDefault(Return(false));
    
    ON_CALL(*mock, isStreamValid(_))
        .WillByDefault(Return(false));
    
    ON_CALL(*mock, hasErrors())
        .WillByDefault(Return(true));
    
    ON_CALL(*mock, getFrameCount())
        .WillByDefault(Return(0));
    
    // Return empty results for failure case
    std::vector<ServiceInfo> emptyServices;
    ON_CALL(*mock, getServices())
        .WillByDefault(Return(emptyServices));
    
    std::vector<std::string> errorMessages = {
        "Invalid ETI frame synchronization",
        "Corrupted frame header",
        "CRC check failed"
    };
    
    ON_CALL(*mock, getErrorMessages())
        .WillByDefault(Return(errorMessages));
    
    EnsembleInfo invalidEnsemble;
    invalidEnsemble.isValid = false;
    ON_CALL(*mock, getEnsembleInfo())
        .WillByDefault(Return(invalidEnsemble));
    
    return mock;
}

std::unique_ptr<MockEnsembleManager> TddMockFactory::createMultiServiceEnsemble() {
    auto mock = std::make_unique<MockEnsembleManager>();
    
    // Create multiple services for realistic testing
    std::vector<ServiceInfo> services;
    
    // Radio 1 - DAB+ service
    ServiceInfo radio1;
    radio1.serviceId = 0x1001;
    radio1.label = "BBC Radio 1";
    radio1.subChannelId = 1;
    radio1.audioType = "DAB+";
    radio1.bitRate = 128;
    radio1.isActive = true;
    services.push_back(radio1);
    
    // Radio 2 - DAB service
    ServiceInfo radio2;
    radio2.serviceId = 0x1002;
    radio2.label = "BBC Radio 2";
    radio2.subChannelId = 2;
    radio2.audioType = "DAB";
    radio2.bitRate = 192;
    radio2.isActive = true;
    services.push_back(radio2);
    
    // Local radio - DAB+ service
    ServiceInfo localRadio;
    localRadio.serviceId = 0x2001;
    localRadio.label = "Local FM";
    localRadio.subChannelId = 3;
    localRadio.audioType = "DAB+";
    localRadio.bitRate = 96;
    localRadio.isActive = false;
    services.push_back(localRadio);
    
    // Classical music - High quality DAB
    ServiceInfo classical;
    classical.serviceId = 0x3001;
    classical.label = "BBC Radio 3";
    classical.subChannelId = 4;
    classical.audioType = "DAB";
    classical.bitRate = 256;
    classical.isActive = true;
    services.push_back(classical);
    
    ON_CALL(*mock, getAllServices())
        .WillByDefault(Return(services));
    
    ON_CALL(*mock, getServiceCount())
        .WillByDefault(Return(services.size()));
    
    // Configure individual service access
    ON_CALL(*mock, getService(0x1001))
        .WillByDefault(Return(radio1));
    ON_CALL(*mock, getService(0x1002))
        .WillByDefault(Return(radio2));
    ON_CALL(*mock, getService(0x2001))
        .WillByDefault(Return(localRadio));
    ON_CALL(*mock, getService(0x3001))
        .WillByDefault(Return(classical));
    
    // Configure service existence checks
    ON_CALL(*mock, hasService(0x1001))
        .WillByDefault(Return(true));
    ON_CALL(*mock, hasService(0x1002))
        .WillByDefault(Return(true));
    ON_CALL(*mock, hasService(0x2001))
        .WillByDefault(Return(true));
    ON_CALL(*mock, hasService(0x3001))
        .WillByDefault(Return(true));
    ON_CALL(*mock, hasService(_))
        .WillByDefault(Return(false)); // Default for unknown services
    
    // Configure service management
    ON_CALL(*mock, addService(_))
        .WillByDefault(Return(true));
    ON_CALL(*mock, removeService(_))
        .WillByDefault(Return(true));
    
    // Configure ensemble information
    EnsembleInfo ensemble;
    ensemble.ensembleId = 0xBBC1;
    ensemble.label = "BBC National DAB";
    ensemble.serviceCount = static_cast<uint8_t>(services.size());
    ensemble.country = "GB";
    ensemble.isValid = true;
    
    ON_CALL(*mock, getEnsembleInfo())
        .WillByDefault(Return(ensemble));
    ON_CALL(*mock, setEnsembleInfo(_))
        .WillByDefault(Return(true));
    
    ON_CALL(*mock, isValid())
        .WillByDefault(Return(true));
    
    return mock;
}

std::unique_ptr<MockAudioDecoder> TddMockFactory::createWorkingAudioDecoder() {
    auto mock = std::make_unique<MockAudioDecoder>();
    
    // Configure successful audio decoding
    ON_CALL(*mock, decode(_))
        .WillByDefault(Return(true));
    
    ON_CALL(*mock, supportsFormat("DAB"))
        .WillByDefault(Return(true));
    ON_CALL(*mock, supportsFormat("DAB+"))
        .WillByDefault(Return(true));
    ON_CALL(*mock, supportsFormat("MP2"))
        .WillByDefault(Return(true));
    ON_CALL(*mock, supportsFormat("AAC"))
        .WillByDefault(Return(true));
    
    // Configure audio properties
    ON_CALL(*mock, getSampleRate())
        .WillByDefault(Return(48000)); // Standard DAB sample rate
    
    ON_CALL(*mock, getBitRate())
        .WillByDefault(Return(128)); // Typical DAB+ bitrate
    
    ON_CALL(*mock, getChannels())
        .WillByDefault(Return(2)); // Stereo
    
    ON_CALL(*mock, getFormat())
        .WillByDefault(Return("DAB+"));
    
    ON_CALL(*mock, isConfigured())
        .WillByDefault(Return(true));
    
    // Generate sample audio data (1 second of silence at 48kHz stereo)
    std::vector<int16_t> sampleAudio(48000 * 2, 0);
    
    // Add some simple test pattern (sine wave for left channel)
    for (size_t i = 0; i < sampleAudio.size(); i += 2) {
        double time = static_cast<double>(i / 2) / 48000.0;
        int16_t sample = static_cast<int16_t>(sin(2.0 * M_PI * 440.0 * time) * 16383.0); // 440Hz tone
        sampleAudio[i] = sample;     // Left channel
        sampleAudio[i + 1] = 0;      // Right channel (silence)
    }
    
    ON_CALL(*mock, getDecodedSamples())
        .WillByDefault(Return(sampleAudio));
    
    ON_CALL(*mock, getDecodedSampleCount())
        .WillByDefault(Return(sampleAudio.size()));
    
    return mock;
}

std::tuple<
    std::unique_ptr<MockEtiProcessor>,
    std::unique_ptr<MockFicDecoder>,
    std::unique_ptr<MockDabDecoder>,
    std::unique_ptr<MockEnsembleManager>
> TddMockFactory::createCompletePipeline() {
    
    // Create processor mock
    auto processor = createSuccessfulProcessor();
    
    // Create FIC decoder mock
    auto ficDecoder = std::make_unique<MockFicDecoder>();
    
    ON_CALL(*ficDecoder, decodeFic(_))
        .WillByDefault(Return(true));
    ON_CALL(*ficDecoder, processFig(_))
        .WillByDefault(Return(true));
    ON_CALL(*ficDecoder, isValidFicBlock(_))
        .WillByDefault(Return(true));
    ON_CALL(*ficDecoder, hasEnsembleInfo())
        .WillByDefault(Return(true));
    
    EnsembleInfo ensemble;
    ensemble.ensembleId = 0xBBC1;
    ensemble.label = "BBC National DAB";
    ensemble.serviceCount = 4;
    ensemble.country = "GB";
    ensemble.isValid = true;
    
    ON_CALL(*ficDecoder, getEnsembleInfo())
        .WillByDefault(Return(ensemble));
    ON_CALL(*ficDecoder, getEnsembleLabel())
        .WillByDefault(Return("BBC National DAB"));
    
    std::vector<ServiceInfo> services;
    ServiceInfo service;
    service.serviceId = 0x1001;
    service.label = "BBC Radio 1";
    services.push_back(service);
    
    ON_CALL(*ficDecoder, getServiceList())
        .WillByDefault(Return(services));
    ON_CALL(*ficDecoder, getServiceLabel(0x1001))
        .WillByDefault(Return("BBC Radio 1"));
    
    // Create DAB decoder mock
    auto dabDecoder = std::make_unique<MockDabDecoder>();
    
    ON_CALL(*dabDecoder, decode(_))
        .WillByDefault(Return(true));
    ON_CALL(*dabDecoder, isValidAudioFrame(_))
        .WillByDefault(Return(true));
    ON_CALL(*dabDecoder, getSampleRate())
        .WillByDefault(Return(48000));
    ON_CALL(*dabDecoder, getChannelCount())
        .WillByDefault(Return(2));
    ON_CALL(*dabDecoder, getAudioType())
        .WillByDefault(Return("DAB+"));
    ON_CALL(*dabDecoder, hasAudioData())
        .WillByDefault(Return(true));
    
    // Sample audio data
    std::vector<int16_t> audioSamples(4800, 0); // 0.1 second of audio
    ON_CALL(*dabDecoder, getAudioSamples())
        .WillByDefault(Return(audioSamples));
    
    // Create ensemble manager mock
    auto ensembleManager = createMultiServiceEnsemble();
    
    return std::make_tuple(
        std::move(processor),
        std::move(ficDecoder),
        std::move(dabDecoder),
        std::move(ensembleManager)
    );
}