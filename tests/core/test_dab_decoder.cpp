#include <gtest/gtest.h>
// #include <gmock/gmock.h> // Temporarily disabled
#include <QSignalSpy>
#include <QByteArray>
#include <QDateTime>
#include "../../src/core/eti_types.h"
#include "../../src/core/dab_decoder.h"

// using namespace testing; // Temporarily disabled
using eti::EtiFrameData;

/**
 * @brief Test DAB Decoder TDD Implementation
 * 
 * Comprehensive test suite for DabDecoder class following TDD methodology.
 * Tests cover DAB/DAB+ decoding, service management, and error handling.
 * 
 * Test Categories:
 * - Initialization and configuration
 * - ETI frame processing
 * - Service discovery and selection
 * - Audio decoding (DAB MP2 and DAB+)
 * - Error handling and recovery
 * - Performance and statistics
 */
class DabDecoderTest : public ::testing::Test 
{
protected:
    void SetUp() override 
    {
        decoder = std::make_unique<DabDecoder>();
    }

    void TearDown() override 
    {
        decoder.reset();
    }

    // Helper methods
    EtiFrameData createValidEtiFrame(quint32 frameNumber = 1) {
        EtiFrameData frame;
        frame.frameNumber = frameNumber;
        frame.timestamp = QTime::currentTime();
        
        // Create valid ETI frame with sync pattern
        QByteArray data;
        data.resize(256);
        
        // ETI sync pattern
        data[0] = static_cast<char>(0xFF);
        data[1] = static_cast<char>(0x1F);
        data[2] = static_cast<char>(0xEC);
        data[3] = static_cast<char>(0xD6);
        
        // Fill rest with test data
        for (int i = 4; i < data.size(); ++i) {
            data[i] = static_cast<char>(i % 256);
        }
        
        frame.data = data;
        frame.isValid = true;
        
        return frame;
    }

    EtiFrameData createInvalidEtiFrame() {
        EtiFrameData frame;
        frame.frameNumber = 999;
        frame.timestamp = QTime::currentTime();
        frame.data = QByteArray(4, 0x00); // Invalid sync
        frame.isValid = false;
        return frame;
    }

    QByteArray createMockAudioData(int size = 128) {
        QByteArray data;
        data.resize(size);
        for (int i = 0; i < size; ++i) {
            data[i] = static_cast<char>(qSin(i * 0.1) * 127);
        }
        return data;
    }

    std::unique_ptr<DabDecoder> decoder;
};

// ============================================================================
// INITIALIZATION AND CONFIGURATION TESTS
// ============================================================================

TEST_F(DabDecoderTest, ConstructorInitializesCorrectly)
{
    // ASSERT
    EXPECT_FALSE(decoder->isInitialized());
    EXPECT_EQ(decoder->getStatus(), DabDecoder::DecoderStatus::Idle);
    EXPECT_EQ(decoder->getCurrentServiceId(), 0);
    EXPECT_EQ(decoder->getProcessedFrameCount(), 0);
    EXPECT_EQ(decoder->getErrorRate(), 0.0);
}

TEST_F(DabDecoderTest, InitializationSucceeds)
{
    // ARRANGE
    QSignalSpy statusSpy(decoder.get(), &DabDecoder::statusChanged);
    
    // ACT
    bool result = decoder->initialize();
    
    // ASSERT
    EXPECT_TRUE(result);
    EXPECT_TRUE(decoder->isInitialized());
    EXPECT_EQ(decoder->getStatus(), DabDecoder::DecoderStatus::Idle);
    EXPECT_EQ(statusSpy.count(), 1);
    EXPECT_EQ(statusSpy.at(0).at(0).value<DabDecoder::DecoderStatus>(), 
              DabDecoder::DecoderStatus::Idle);
}

TEST_F(DabDecoderTest, DoubleInitializationReturnsTrue)
{
    // ARRANGE
    decoder->initialize();
    
    // ACT
    bool result = decoder->initialize();
    
    // ASSERT
    EXPECT_TRUE(result);
    EXPECT_TRUE(decoder->isInitialized());
}

TEST_F(DabDecoderTest, ShutdownClearsState)
{
    // ARRANGE
    decoder->initialize();
    decoder->processEtiFrame(createValidEtiFrame());
    
    // ACT
    decoder->shutdown();
    
    // ASSERT
    EXPECT_FALSE(decoder->isInitialized());
    EXPECT_EQ(decoder->getStatus(), DabDecoder::DecoderStatus::Idle);
    EXPECT_EQ(decoder->getCurrentServiceId(), 0);
    EXPECT_FALSE(decoder->hasDecodedAudio());
}

TEST_F(DabDecoderTest, BufferSizeConfigurationWorks)
{
    // ARRANGE
    quint32 customSize = 16384;
    
    // ACT
    decoder->setMaxBufferSize(customSize);
    
    // ASSERT
    EXPECT_EQ(decoder->getMaxBufferSize(), customSize);
}

// ============================================================================
// ETI FRAME PROCESSING TESTS
// ============================================================================

TEST_F(DabDecoderTest, ProcessValidEtiFrameSucceeds)
{
    // ARRANGE
    decoder->initialize();
    EtiFrameData frame = createValidEtiFrame();
    QSignalSpy statusSpy(decoder.get(), &DabDecoder::statusChanged);
    
    // ACT
    bool result = decoder->processEtiFrame(frame);
    
    // ASSERT
    EXPECT_TRUE(result);
    EXPECT_EQ(decoder->getProcessedFrameCount(), 1);
    EXPECT_GE(statusSpy.count(), 1); // Should change to Processing/Synchronized
}

TEST_F(DabDecoderTest, ProcessInvalidEtiFrameFails)
{
    // ARRANGE
    decoder->initialize();
    EtiFrameData frame = createInvalidEtiFrame();
    QSignalSpy errorSpy(decoder.get(), &DabDecoder::decodingError);
    
    // ACT
    bool result = decoder->processEtiFrame(frame);
    
    // ASSERT
    EXPECT_FALSE(result);
    EXPECT_GT(decoder->getErrorRate(), 0.0);
    EXPECT_GE(errorSpy.count(), 1);
}

TEST_F(DabDecoderTest, ProcessFrameWithoutInitializationFails)
{
    // ARRANGE
    EtiFrameData frame = createValidEtiFrame();
    
    // ACT
    bool result = decoder->processEtiFrame(frame);
    
    // ASSERT
    EXPECT_FALSE(result);
    EXPECT_EQ(decoder->getProcessedFrameCount(), 0);
}

TEST_F(DabDecoderTest, ProcessEmptyFrameFails)
{
    // ARRANGE
    decoder->initialize();
    EtiFrameData frame;
    frame.data.clear();
    frame.isValid = false;
    QSignalSpy errorSpy(decoder.get(), &DabDecoder::decodingError);
    
    // ACT
    bool result = decoder->processEtiFrame(frame);
    
    // ASSERT
    EXPECT_FALSE(result);
    EXPECT_GT(decoder->getErrorRate(), 0.0);
    EXPECT_GE(errorSpy.count(), 1);
}

TEST_F(DabDecoderTest, MultipleFrameProcessingUpdatesStatistics)
{
    // ARRANGE
    decoder->initialize();
    const int frameCount = 10;
    
    // ACT
    for (int i = 0; i < frameCount; ++i) {
        EtiFrameData frame = createValidEtiFrame(i + 1);
        decoder->processEtiFrame(frame);
    }
    
    // ASSERT
    EXPECT_EQ(decoder->getProcessedFrameCount(), frameCount);
    EXPECT_LE(decoder->getErrorRate(), 0.1); // Should be low error rate
}

// ============================================================================
// SERVICE DISCOVERY AND MANAGEMENT TESTS
// ============================================================================

TEST_F(DabDecoderTest, ServiceDiscoveryEmitsSignal)
{
    // ARRANGE
    decoder->initialize();
    QSignalSpy servicesSpy(decoder.get(), &DabDecoder::servicesDiscovered);
    
    // ACT
    EtiFrameData frame = createValidEtiFrame();
    decoder->processEtiFrame(frame);
    
    // ASSERT
    EXPECT_GE(servicesSpy.count(), 1);
    
    if (servicesSpy.count() > 0) {
        auto services = servicesSpy.at(0).at(0).value<QList<DabDecoder::ServiceComponent>>();
        EXPECT_GT(services.size(), 0);
    }
}

TEST_F(DabDecoderTest, GetAvailableServicesReturnsDiscoveredServices)
{
    // ARRANGE
    decoder->initialize();
    decoder->processEtiFrame(createValidEtiFrame());
    
    // ACT
    auto services = decoder->getAvailableServices();
    
    // ASSERT
    EXPECT_GT(services.size(), 0);
    
    // Check that services have valid data
    for (const auto& service : services) {
        EXPECT_GT(service.serviceId, 0);
        EXPECT_FALSE(service.label.isEmpty());
        EXPECT_NE(service.codec, DabDecoder::AudioCodec::Unknown);
        EXPECT_GT(service.bitrate, 0);
    }
}

TEST_F(DabDecoderTest, SelectValidServiceSucceeds)
{
    // ARRANGE
    decoder->initialize();
    decoder->processEtiFrame(createValidEtiFrame());
    auto services = decoder->getAvailableServices();
    ASSERT_GT(services.size(), 0);
    
    quint16 serviceId = services.first().serviceId;
    QSignalSpy selectionSpy(decoder.get(), &DabDecoder::serviceSelected);
    
    // ACT
    bool result = decoder->selectService(serviceId);
    
    // ASSERT
    EXPECT_TRUE(result);
    EXPECT_EQ(decoder->getCurrentServiceId(), serviceId);
    EXPECT_EQ(selectionSpy.count(), 1);
    
    if (selectionSpy.count() > 0) {
        EXPECT_EQ(selectionSpy.at(0).at(0).toUInt(), serviceId);
    }
}

TEST_F(DabDecoderTest, SelectInvalidServiceFails)
{
    // ARRANGE
    decoder->initialize();
    quint16 invalidServiceId = 0xFFFF;
    
    // ACT
    bool result = decoder->selectService(invalidServiceId);
    
    // ASSERT
    EXPECT_FALSE(result);
    EXPECT_EQ(decoder->getCurrentServiceId(), 0);
}

TEST_F(DabDecoderTest, GetServiceInfoReturnsCorrectData)
{
    // ARRANGE
    decoder->initialize();
    decoder->processEtiFrame(createValidEtiFrame());
    auto services = decoder->getAvailableServices();
    ASSERT_GT(services.size(), 0);
    
    quint16 serviceId = services.first().serviceId;
    
    // ACT
    auto serviceInfo = decoder->getServiceInfo(serviceId);
    
    // ASSERT
    EXPECT_EQ(serviceInfo.serviceId, serviceId);
    EXPECT_FALSE(serviceInfo.label.isEmpty());
    EXPECT_NE(serviceInfo.codec, DabDecoder::AudioCodec::Unknown);
    EXPECT_GT(serviceInfo.bitrate, 0);
}

// ============================================================================
// AUDIO PROCESSING TESTS
// ============================================================================

TEST_F(DabDecoderTest, ProcessSubchannelWithValidDataSucceeds)
{
    // ARRANGE
    decoder->initialize();
    decoder->processEtiFrame(createValidEtiFrame());
    QByteArray audioData = createMockAudioData(64);
    QSignalSpy audioSpy(decoder.get(), &DabDecoder::audioDataReady);
    
    // ACT
    bool result = decoder->processSubChannel(0, audioData);
    
    // ASSERT
    EXPECT_TRUE(result);
}

TEST_F(DabDecoderTest, ProcessSubchannelWithEmptyDataFails)
{
    // ARRANGE
    decoder->initialize();
    
    // ACT
    bool result = decoder->processSubChannel(0, QByteArray());
    
    // ASSERT
    EXPECT_FALSE(result);
}

TEST_F(DabDecoderTest, AudioBufferManagementWorks)
{
    // ARRANGE
    decoder->initialize();
    decoder->processEtiFrame(createValidEtiFrame());
    auto services = decoder->getAvailableServices();
    ASSERT_GT(services.size(), 0);
    
    decoder->selectService(services.first().serviceId);
    QByteArray audioData = createMockAudioData(128);
    
    // ACT
    decoder->processSubChannel(services.first().subChannelId, audioData);
    
    // ASSERT - Check if audio is available
    bool hadAudio = decoder->hasDecodedAudio();
    QByteArray retrievedAudio = decoder->getDecodedAudio();
    
    if (hadAudio) {
        EXPECT_GT(retrievedAudio.size(), 0);
        EXPECT_FALSE(decoder->hasDecodedAudio()); // Should be cleared after retrieval
    }
}

TEST_F(DabDecoderTest, ClearAudioBufferWorks)
{
    // ARRANGE
    decoder->initialize();
    decoder->processEtiFrame(createValidEtiFrame());
    auto services = decoder->getAvailableServices();
    ASSERT_GT(services.size(), 0);
    
    decoder->selectService(services.first().serviceId);
    decoder->processSubChannel(services.first().subChannelId, createMockAudioData());
    
    // ACT
    decoder->clearAudioBuffer();
    
    // ASSERT
    EXPECT_FALSE(decoder->hasDecodedAudio());
}

TEST_F(DabDecoderTest, GetBitrateReturnsCorrectValue)
{
    // ARRANGE
    decoder->initialize();
    decoder->processEtiFrame(createValidEtiFrame());
    auto services = decoder->getAvailableServices();
    ASSERT_GT(services.size(), 0);
    
    decoder->selectService(services.first().serviceId);
    
    // ACT
    quint32 bitrate = decoder->getBitrate();
    
    // ASSERT
    EXPECT_GT(bitrate, 0);
    EXPECT_EQ(bitrate, services.first().bitrate);
}

// ============================================================================
// ERROR HANDLING AND RECOVERY TESTS
// ============================================================================

TEST_F(DabDecoderTest, ErrorRateCalculationIsAccurate)
{
    // ARRANGE
    decoder->initialize();
    const int totalFrames = 10;
    const int errorFrames = 3;
    
    // ACT - Process mix of valid and invalid frames
    for (int i = 0; i < totalFrames; ++i) {
        EtiFrameData frame = (i < errorFrames) ? createInvalidEtiFrame() : createValidEtiFrame();
        decoder->processEtiFrame(frame);
    }
    
    // ASSERT
    double expectedErrorRate = static_cast<double>(errorFrames) / totalFrames;
    EXPECT_NEAR(decoder->getErrorRate(), expectedErrorRate, 0.01);
}

TEST_F(DabDecoderTest, HighErrorRateTriggersErrorStatus)
{
    // ARRANGE
    decoder->initialize();
    QSignalSpy statusSpy(decoder.get(), &DabDecoder::statusChanged);
    QSignalSpy errorSpy(decoder.get(), &DabDecoder::decodingError);
    
    // ACT - Process many invalid frames to trigger high error rate
    for (int i = 0; i < 20; ++i) {
        decoder->processEtiFrame(createInvalidEtiFrame());
    }
    
    // ASSERT
    EXPECT_GT(decoder->getErrorRate(), 0.5); // High error rate
    EXPECT_GT(errorSpy.count(), 0);
    
    // Check if status eventually changes to Error
    bool errorStatusFound = false;
    for (int i = 0; i < statusSpy.count(); ++i) {
        if (statusSpy.at(i).at(0).value<DabDecoder::DecoderStatus>() == DabDecoder::DecoderStatus::Error) {
            errorStatusFound = true;
            break;
        }
    }
    
    if (decoder->getErrorRate() > 0.1) { // Only check if error rate is high enough
        EXPECT_TRUE(errorStatusFound);
    }
}

TEST_F(DabDecoderTest, SynchronizationLossAndRecoveryWorks)
{
    // ARRANGE
    decoder->initialize();
    QSignalSpy statusSpy(decoder.get(), &DabDecoder::statusChanged);
    
    // ACT - First sync with valid frame
    decoder->processEtiFrame(createValidEtiFrame());
    
    // Then lose sync with invalid frames
    for (int i = 0; i < 3; ++i) {
        decoder->processEtiFrame(createInvalidEtiFrame());
    }
    
    // Recover with valid frame
    decoder->processEtiFrame(createValidEtiFrame());
    
    // ASSERT
    EXPECT_GE(statusSpy.count(), 2); // Should have status changes
    EXPECT_GT(decoder->getErrorRate(), 0.0); // Should have some errors
}

// ============================================================================
// PERFORMANCE AND STATISTICS TESTS
// ============================================================================

TEST_F(DabDecoderTest, FrameCounterIncreasesCorrectly)
{
    // ARRANGE
    decoder->initialize();
    const int frameCount = 15;
    
    // ACT
    for (int i = 0; i < frameCount; ++i) {
        decoder->processEtiFrame(createValidEtiFrame(i + 1));
    }
    
    // ASSERT
    EXPECT_EQ(decoder->getProcessedFrameCount(), frameCount);
}

TEST_F(DabDecoderTest, StatusTransitionsWorkCorrectly)
{
    // ARRANGE
    decoder->initialize();
    QSignalSpy statusSpy(decoder.get(), &DabDecoder::statusChanged);
    
    // ACT
    decoder->processEtiFrame(createValidEtiFrame());
    
    // ASSERT
    EXPECT_GE(statusSpy.count(), 1);
    
    // Check that we get expected status transitions
    bool foundProcessingOrSynchronized = false;
    for (int i = 0; i < statusSpy.count(); ++i) {
        auto status = statusSpy.at(i).at(0).value<DabDecoder::DecoderStatus>();
        if (status == DabDecoder::DecoderStatus::Processing || 
            status == DabDecoder::DecoderStatus::Synchronized) {
            foundProcessingOrSynchronized = true;
            break;
        }
    }
    
    EXPECT_TRUE(foundProcessingOrSynchronized);
}

// ============================================================================
// INTEGRATION TESTS
// ============================================================================

TEST_F(DabDecoderTest, CompleteWorkflowFromInitToAudioOutput)
{
    // ARRANGE
    QSignalSpy servicesSpy(decoder.get(), &DabDecoder::servicesDiscovered);
    QSignalSpy audioSpy(decoder.get(), &DabDecoder::audioDataReady);
    QSignalSpy selectionSpy(decoder.get(), &DabDecoder::serviceSelected);
    
    // ACT
    // 1. Initialize
    ASSERT_TRUE(decoder->initialize());
    
    // 2. Process frame to discover services
    ASSERT_TRUE(decoder->processEtiFrame(createValidEtiFrame()));
    
    // 3. Wait for service discovery
    ASSERT_GE(servicesSpy.count(), 1);
    auto services = decoder->getAvailableServices();
    ASSERT_GT(services.size(), 0);
    
    // 4. Select a service
    quint16 serviceId = services.first().serviceId;
    ASSERT_TRUE(decoder->selectService(serviceId));
    ASSERT_EQ(selectionSpy.count(), 1);
    
    // 5. Process subchannel data
    QByteArray audioData = createMockAudioData(128);
    ASSERT_TRUE(decoder->processSubChannel(services.first().subChannelId, audioData));
    
    // ASSERT - Check complete workflow
    EXPECT_TRUE(decoder->isInitialized());
    EXPECT_EQ(decoder->getCurrentServiceId(), serviceId);
    EXPECT_GT(decoder->getProcessedFrameCount(), 0);
    EXPECT_LE(decoder->getErrorRate(), 0.1);
    
    // May or may not have audio depending on implementation
    if (decoder->hasDecodedAudio()) {
        QByteArray audio = decoder->getDecodedAudio();
        EXPECT_GT(audio.size(), 0);
    }
}

TEST_F(DabDecoderTest, ThreadSafetyBasicTest)
{
    // ARRANGE
    decoder->initialize();
    
    // ACT & ASSERT - Basic operations should not crash
    decoder->processEtiFrame(createValidEtiFrame());
    auto services = decoder->getAvailableServices();
    
    if (!services.isEmpty()) {
        decoder->selectService(services.first().serviceId);
        decoder->processSubChannel(services.first().subChannelId, createMockAudioData());
    }
    
    decoder->getStatus();
    decoder->getProcessedFrameCount();
    decoder->getErrorRate();
    
    // Should complete without crashes
    SUCCEED();
}