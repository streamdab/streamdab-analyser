#pragma once

#include <gmock/gmock.h>
#include <vector>
#include <string>
#include <memory>
#include <QByteArray>
#include <QString>

// Include actual type definitions from core headers
#include "../../src/core/eti_types.h"
#include "../../src/core/fic_decoder.h"
#include "../../src/core/eti_processor.h"

/**
 * @file mock_eti_processor.h
 * @brief Google Mock implementations for ETI processing components
 * 
 * These mocks enable isolated unit testing following TDD methodology.
 * Used for Red-Green-Refactor cycles with dependency isolation.
 */

// Type aliases from the actual implementation
using EtiFrame = eti::EtiFrame;
using EtiFrameData = eti::EtiFrameData;
using ServiceInfo = FicDecoder::ServiceInfo;
using EnsembleInfo = FicDecoder::EnsembleInfo;

// Define ProcessResult for mock compatibility
struct ProcessResult {
    bool success = false;
    std::string errorMessage;
    size_t framesProcessed = 0;
    size_t servicesFound = 0;
};

/**
 * @brief Mock ETI Processor for TDD unit testing
 * 
 * This mock allows testing of components that depend on ETI processing
 * without requiring actual ETI processing implementation.
 */
class MockEtiProcessor : public EtiProcessor {
public:
    MockEtiProcessor(QObject *parent = nullptr) : EtiProcessor(parent) {}
    virtual ~MockEtiProcessor() = default;
    
    // Mock methods using Google Mock
    MOCK_METHOD(ProcessResult, process, (const std::vector<uint8_t>& etiData), ());
    MOCK_METHOD(ProcessResult, processFrame, (const EtiFrame& frame), ());
    MOCK_METHOD(bool, isValidFrame, (const EtiFrame& frame), (const));
    MOCK_METHOD(bool, isStreamValid, (const std::vector<uint8_t>& stream), (const));
    MOCK_METHOD(void, reset, (), ());
    MOCK_METHOD(void, configure, (const std::string& config), ());
    MOCK_METHOD(std::vector<ServiceInfo>, getServices, (), (const));
    MOCK_METHOD(EnsembleInfo, getEnsembleInfo, (), (const));
    MOCK_METHOD(size_t, getFrameCount, (), (const));
    MOCK_METHOD(bool, hasErrors, (), (const));
    MOCK_METHOD(std::vector<std::string>, getErrorMessages, (), (const));
    
    // Test helper methods
    MOCK_METHOD(void, setTestFrame, (const EtiFrame& frame), ());
    MOCK_METHOD(EtiFrame, getTestFrame, (), (const));
};

/**
 * @brief Mock DAB Decoder for audio service decoding
 */
class MockDabDecoder {
public:
    virtual ~MockDabDecoder() = default;
    
    MOCK_METHOD(bool, decode, (const std::vector<uint8_t>& audioData), ());
    MOCK_METHOD(bool, isValidAudioFrame, (const std::vector<uint8_t>& frame), (const));
    MOCK_METHOD(std::vector<int16_t>, getAudioSamples, (), (const));
    MOCK_METHOD(int, getSampleRate, (), (const));
    MOCK_METHOD(int, getChannelCount, (), (const));
    MOCK_METHOD(std::string, getAudioType, (), (const)); // "DAB" or "DAB+"
    MOCK_METHOD(void, reset, (), ());
    MOCK_METHOD(bool, hasAudioData, (), (const));
};

/**
 * @brief Mock FIC (Fast Information Channel) Decoder
 */
class MockFicDecoder {
public:
    virtual ~MockFicDecoder() = default;
    
    MOCK_METHOD(bool, decodeFic, (const std::vector<uint8_t>& ficData), ());
    MOCK_METHOD(bool, processFig, (const std::vector<uint8_t>& figData), ());
    MOCK_METHOD(bool, isValidFicBlock, (const std::vector<uint8_t>& block), (const));
    MOCK_METHOD(EnsembleInfo, getEnsembleInfo, (), (const));
    MOCK_METHOD(std::vector<ServiceInfo>, getServiceList, (), (const));
    MOCK_METHOD(std::string, getServiceLabel, (uint32_t serviceId), (const));
    MOCK_METHOD(std::string, getEnsembleLabel, (), (const));
    MOCK_METHOD(void, reset, (), ());
    MOCK_METHOD(bool, hasEnsembleInfo, (), (const));
};

/**
 * @brief Mock Ensemble Manager for service coordination
 */
class MockEnsembleManager {
public:
    virtual ~MockEnsembleManager() = default;
    
    MOCK_METHOD(bool, addService, (const ServiceInfo& service), ());
    MOCK_METHOD(bool, removeService, (uint32_t serviceId), ());
    MOCK_METHOD(ServiceInfo, getService, (uint32_t serviceId), (const));
    MOCK_METHOD(std::vector<ServiceInfo>, getAllServices, (), (const));
    MOCK_METHOD(size_t, getServiceCount, (), (const));
    MOCK_METHOD(bool, hasService, (uint32_t serviceId), (const));
    MOCK_METHOD(void, clear, (), ());
    MOCK_METHOD(bool, setEnsembleInfo, (const EnsembleInfo& info), ());
    MOCK_METHOD(EnsembleInfo, getEnsembleInfo, (), (const));
    MOCK_METHOD(bool, isValid, (), (const));
};

/**
 * @brief Mock Service Manager for individual service handling
 */
class MockServiceManager {
public:
    virtual ~MockServiceManager() = default;
    
    MOCK_METHOD(bool, startService, (uint32_t serviceId), ());
    MOCK_METHOD(bool, stopService, (uint32_t serviceId), ());
    MOCK_METHOD(bool, isServiceActive, (uint32_t serviceId), (const));
    MOCK_METHOD(std::vector<uint32_t>, getActiveServices, (), (const));
    MOCK_METHOD(bool, processServiceData, (uint32_t serviceId, const std::vector<uint8_t>& data), ());
    MOCK_METHOD(std::vector<uint8_t>, getServiceAudioData, (uint32_t serviceId), (const));
    MOCK_METHOD(bool, hasAudioData, (uint32_t serviceId), (const));
    MOCK_METHOD(void, reset, (), ());
};

/**
 * @brief Mock Audio Decoder for audio processing
 */
class MockAudioDecoder {
public:
    virtual ~MockAudioDecoder() = default;
    
    MOCK_METHOD(bool, decode, (const std::vector<uint8_t>& audioData), ());
    MOCK_METHOD(bool, supportsFormat, (const std::string& format), (const));
    MOCK_METHOD(std::vector<int16_t>, getDecodedSamples, (), (const));
    MOCK_METHOD(int, getSampleRate, (), (const));
    MOCK_METHOD(int, getBitRate, (), (const));
    MOCK_METHOD(int, getChannels, (), (const));
    MOCK_METHOD(std::string, getFormat, (), (const));
    MOCK_METHOD(bool, isConfigured, (), (const));
    MOCK_METHOD(void, reset, (), ());
    MOCK_METHOD(size_t, getDecodedSampleCount, (), (const));
};

/**
 * @brief Mock File Reader for ETI file I/O
 */
class MockFileReader {
public:
    virtual ~MockFileReader() = default;
    
    MOCK_METHOD(bool, open, (const std::string& filename), ());
    MOCK_METHOD(void, close, (), ());
    MOCK_METHOD(bool, isOpen, (), (const));
    MOCK_METHOD(std::vector<uint8_t>, readFrame, (), ());
    MOCK_METHOD(std::vector<uint8_t>, readAll, (), ());
    MOCK_METHOD(bool, seekToFrame, (size_t frameNumber), ());
    MOCK_METHOD(size_t, getFrameCount, (), (const));
    MOCK_METHOD(size_t, getCurrentFrameNumber, (), (const));
    MOCK_METHOD(bool, hasMoreFrames, (), (const));
    MOCK_METHOD(bool, isValidEtiFile, (const std::string& filename), (const));
};

/**
 * @brief Mock Network Stream Receiver
 */
class MockNetworkReceiver {
public:
    virtual ~MockNetworkReceiver() = default;
    
    MOCK_METHOD(bool, connect, (const std::string& host, uint16_t port), ());
    MOCK_METHOD(void, disconnect, (), ());
    MOCK_METHOD(bool, isConnected, (), (const));
    MOCK_METHOD(std::vector<uint8_t>, receiveData, (size_t maxBytes), ());
    MOCK_METHOD(bool, hasData, (), (const));
    MOCK_METHOD(void, setReceiveTimeout, (int milliseconds), ());
    MOCK_METHOD(std::string, getLastError, (), (const));
    MOCK_METHOD(size_t, getBytesReceived, (), (const));
    MOCK_METHOD(void, reset, (), ());
};

/**
 * @brief TDD Mock Factory for creating configured mocks
 * 
 * This factory provides pre-configured mocks for common TDD scenarios.
 */
class TddMockFactory {
public:
    /**
     * @brief Create a mock ETI processor for successful processing
     * @return Configured mock that returns success for processing calls
     */
    static std::unique_ptr<MockEtiProcessor> createSuccessfulProcessor();
    
    /**
     * @brief Create a mock ETI processor that simulates failures
     * @return Configured mock that returns failures for processing calls
     */
    static std::unique_ptr<MockEtiProcessor> createFailingProcessor();
    
    /**
     * @brief Create a mock with realistic DAB ensemble data
     * @return Configured mock with multi-service ensemble
     */
    static std::unique_ptr<MockEnsembleManager> createMultiServiceEnsemble();
    
    /**
     * @brief Create a mock audio decoder for successful decoding
     * @return Configured mock that successfully decodes audio
     */
    static std::unique_ptr<MockAudioDecoder> createWorkingAudioDecoder();
    
    /**
     * @brief Create mocks for complete ETI processing pipeline
     * @return Tuple of configured mocks for end-to-end testing
     */
    static std::tuple<
        std::unique_ptr<MockEtiProcessor>,
        std::unique_ptr<MockFicDecoder>,
        std::unique_ptr<MockDabDecoder>,
        std::unique_ptr<MockEnsembleManager>
    > createCompletePipeline();
};

// Note: Type definitions are imported from actual core headers
// - eti::EtiFrame and eti::EtiFrameData from eti_types.h
// - FicDecoder::ServiceInfo and FicDecoder::EnsembleInfo from fic_decoder.h
// - ProcessResult defined above for mock compatibility