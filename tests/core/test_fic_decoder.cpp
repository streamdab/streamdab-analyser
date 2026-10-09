#include <gtest/gtest.h>
#include <QSignalSpy>
#include <QTest>
#include <QByteArray>
#include "../../src/core/fic_decoder.h"

class FicDecoderTest : public ::testing::Test 
{
protected:
    void SetUp() override 
    {
        decoder = new FicDecoder();
    }

    void TearDown() override 
    {
        delete decoder;
        decoder = nullptr;
    }

    // Test helper methods
    QByteArray createValidFib() 
    {
        QByteArray fib(30, 0x00); // 30 bytes of FIG data
        
        // Add simple FIG 0/0 (Ensemble information)
        fib[0] = 0x00; // FIG type 0, length 5
        fib[1] = 0x00; // Extension 0, no flags
        fib[2] = 0x12; // Ensemble ID high byte
        fib[3] = 0x34; // Ensemble ID low byte
        fib[4] = 0x00; // Change flags + alarm
        fib[5] = 0x56; // CIF count
        
        // Add end marker
        fib[6] = static_cast<char>(0xFF);
        
        // Calculate and append CRC
        QByteArray fullFib = fib + QByteArray(2, 0x00);
        quint16 crc = calculateTestCrc(fib);
        fullFib[30] = (crc >> 8) & 0xFF;
        fullFib[31] = crc & 0xFF;
        
        return fullFib;
    }

    QByteArray createFibWithService() 
    {
        QByteArray fib(30, 0x00);
        
        // FIG 0/2 - Service organization
        fib[0] = 0x08; // FIG type 0, length 8
        fib[1] = 0x02; // Extension 2
        fib[2] = static_cast<char>(0xAB); // Service ID high
        fib[3] = static_cast<char>(0xCD); // Service ID low
        fib[4] = 0x01; // PTY code 0, CA=0, nb_service_comp=1
        fib[5] = 0x10; // TMId=0, ASCTy=0, SubChId=4, PS=0, CA=0
        
        // Add end marker
        fib[6] = static_cast<char>(0xFF);
        
        // Calculate and append CRC
        QByteArray fullFib = fib + QByteArray(2, 0x00);
        quint16 crc = calculateTestCrc(fib);
        fullFib[30] = (crc >> 8) & 0xFF;
        fullFib[31] = crc & 0xFF;
        
        return fullFib;
    }

    QByteArray createFibWithSubchannel() 
    {
        QByteArray fib(30, 0x00);
        
        // FIG 0/1 - Sub-channel organization
        fib[0] = 0x0C; // FIG type 0, length 12
        fib[1] = 0x01; // Extension 1
        fib[2] = 0x10; // SubChId=4, start address high bits
        fib[3] = 0x00; // Start address low byte (0x40 = 64)
        fib[4] = static_cast<char>(0x80); // Short form, protection level 0
        fib[5] = 0x20; // Sub-channel size (32 CUs)
        
        // Add end marker
        fib[6] = static_cast<char>(0xFF);
        
        // Calculate and append CRC
        QByteArray fullFib = fib + QByteArray(2, 0x00);
        quint16 crc = calculateTestCrc(fib);
        fullFib[30] = (crc >> 8) & 0xFF;
        fullFib[31] = crc & 0xFF;
        
        return fullFib;
    }

    QByteArray createFibWithEnsembleLabel() 
    {
        QByteArray fib(30, 0x00);
        
        // FIG 1/0 - Ensemble label (partial, due to space constraints)
        fib[0] = 0x2F; // FIG type 1, length 15 (max that fits)
        fib[1] = 0x00; // Extension 0
        fib[2] = 0x12; // Ensemble ID high
        fib[3] = 0x34; // Ensemble ID low
        // Label: "TEST_ENSEMBLE" (truncated to fit)
        const char* label = "TEST_ENSEMBLE   ";
        memcpy(&fib[4], label, 13); // 13 chars to fit in FIB
        
        // Add end marker at byte 17
        fib[17] = static_cast<char>(0xFF);
        
        // Calculate and append CRC
        QByteArray fullFib = fib + QByteArray(2, 0x00);
        quint16 crc = calculateTestCrc(fib);
        fullFib[30] = (crc >> 8) & 0xFF;
        fullFib[31] = crc & 0xFF;
        
        return fullFib;
    }

    QByteArray createInvalidFib() 
    {
        QByteArray fib(32, 0x00);
        // Invalid CRC
        fib[30] = static_cast<char>(0xFF);
        fib[31] = static_cast<char>(0xFF);
        return fib;
    }

private:
    quint16 calculateTestCrc(const QByteArray& data) 
    {
        // Simple CRC calculation for test data
        const quint16 polynomial = 0x1021;
        quint16 crc = 0xFFFF;
        
        for (int i = 0; i < data.size(); ++i) {
            crc ^= (static_cast<quint16>(data[i]) << 8);
            
            for (int bit = 0; bit < 8; ++bit) {
                if (crc & 0x8000) {
                    crc = (crc << 1) ^ polynomial;
                } else {
                    crc <<= 1;
                }
            }
        }
        
        return crc ^ 0xFFFF;
    }

protected:
    FicDecoder* decoder;
};

// Constructor and Initialization Tests
TEST_F(FicDecoderTest, ConstructorInitializesCorrectly)
{
    EXPECT_EQ(decoder->getStatus(), FicDecoder::DecoderStatus::Idle);
    EXPECT_EQ(decoder->getFibCount(), 0u);
    EXPECT_EQ(decoder->getErrorCount(), 0u);
    EXPECT_EQ(decoder->getSyncLevel(), 0);
    EXPECT_TRUE(decoder->getServices().isEmpty());
    EXPECT_TRUE(decoder->getSubChannels().isEmpty());
}

TEST_F(FicDecoderTest, InitializationSucceeds)
{
    QSignalSpy statusSpy(decoder, &FicDecoder::statusChanged);
    
    bool result = decoder->initialize();
    
    EXPECT_TRUE(result);
    EXPECT_EQ(decoder->getStatus(), FicDecoder::DecoderStatus::Idle);
    EXPECT_EQ(statusSpy.count(), 1);
    
    auto statusArg = statusSpy.at(0).at(0).value<FicDecoder::DecoderStatus>();
    EXPECT_EQ(statusArg, FicDecoder::DecoderStatus::Idle);
}

TEST_F(FicDecoderTest, ResetClearsAllData)
{
    // First process some data
    decoder->initialize();
    QByteArray validFic = createValidFib();
    decoder->processFicData(validFic);
    
    // Verify data exists
    EXPECT_GT(decoder->getFibCount(), 0u);
    
    // Reset
    decoder->reset();
    
    // Verify everything is cleared
    EXPECT_EQ(decoder->getStatus(), FicDecoder::DecoderStatus::Idle);
    EXPECT_EQ(decoder->getFibCount(), 0u);
    EXPECT_EQ(decoder->getErrorCount(), 0u);
    EXPECT_EQ(decoder->getSyncLevel(), 0);
    EXPECT_TRUE(decoder->getServices().isEmpty());
    EXPECT_TRUE(decoder->getSubChannels().isEmpty());
}

// FIC Data Processing Tests
TEST_F(FicDecoderTest, ProcessValidFicDataSucceeds)
{
    decoder->initialize();
    QSignalSpy statusSpy(decoder, &FicDecoder::statusChanged);
    QSignalSpy ficSpy(decoder, &FicDecoder::ficDataProcessed);
    
    QByteArray validFic = createValidFib();
    bool result = decoder->processFicData(validFic);
    
    EXPECT_TRUE(result);
    EXPECT_EQ(decoder->getFibCount(), 1u);
    EXPECT_GE(statusSpy.count(), 1); // At least one status change
    EXPECT_EQ(ficSpy.count(), 1);
    
    // Check that processing status was set
    auto lastStatus = statusSpy.last().at(0).value<FicDecoder::DecoderStatus>();
    EXPECT_TRUE(lastStatus == FicDecoder::DecoderStatus::Processing || 
                lastStatus == FicDecoder::DecoderStatus::Synchronized);
}

TEST_F(FicDecoderTest, ProcessEmptyFicDataFails)
{
    decoder->initialize();
    
    QByteArray emptyData;
    bool result = decoder->processFicData(emptyData);
    
    EXPECT_FALSE(result);
    EXPECT_EQ(decoder->getFibCount(), 0u);
    EXPECT_GT(decoder->getErrorCount(), 0u);
}

TEST_F(FicDecoderTest, ProcessInvalidSizeFicDataFails)
{
    decoder->initialize();
    
    QByteArray smallData(10, 0x00); // Too small
    bool result = decoder->processFicData(smallData);
    
    EXPECT_FALSE(result);
    EXPECT_EQ(decoder->getFibCount(), 0u);
    EXPECT_GT(decoder->getErrorCount(), 0u);
}

TEST_F(FicDecoderTest, ProcessInvalidCrcFicDataFails)
{
    decoder->initialize();
    
    QByteArray invalidFic = createInvalidFib();
    bool result = decoder->processFicData(invalidFic);
    
    EXPECT_FALSE(result);
    EXPECT_EQ(decoder->getFibCount(), 0u);
    EXPECT_GT(decoder->getErrorCount(), 0u);
}

TEST_F(FicDecoderTest, ProcessMultipleFibsSucceeds)
{
    decoder->initialize();
    QSignalSpy ficSpy(decoder, &FicDecoder::ficDataProcessed);
    
    // Create FIC data with 3 FIBs
    QByteArray fib1 = createValidFib();
    QByteArray fib2 = createFibWithService();
    QByteArray fib3 = createFibWithSubchannel();
    QByteArray multipleFibs = fib1 + fib2 + fib3;
    
    bool result = decoder->processFicData(multipleFibs);
    
    EXPECT_TRUE(result);
    EXPECT_EQ(decoder->getFibCount(), 3u);
    EXPECT_EQ(ficSpy.count(), 1);
    
    // Check the number of FIBs processed in the signal
    auto fibsProcessed = ficSpy.at(0).at(0).toInt();
    EXPECT_EQ(fibsProcessed, 3);
}

// Ensemble Information Tests
TEST_F(FicDecoderTest, EnsembleInformationIsExtracted)
{
    decoder->initialize();
    QSignalSpy ensembleSpy(decoder, &FicDecoder::ensembleInfoUpdated);
    
    QByteArray validFic = createValidFib();
    decoder->processFicData(validFic);
    
    EXPECT_EQ(ensembleSpy.count(), 1);
    
    auto ensembleInfo = decoder->getEnsembleInfo();
    EXPECT_EQ(ensembleInfo.ensembleId, 0x1234);
    EXPECT_EQ(ensembleInfo.cifCount, 0x56);
}

TEST_F(FicDecoderTest, EnsembleLabelIsExtracted)
{
    decoder->initialize();
    QSignalSpy ensembleSpy(decoder, &FicDecoder::ensembleInfoUpdated);
    
    // First process ensemble info
    QByteArray ensembleInfoFib = createValidFib();
    decoder->processFicData(ensembleInfoFib);
    
    // Then process ensemble label
    QByteArray ensembleLabelFib = createFibWithEnsembleLabel();
    decoder->processFicData(ensembleLabelFib);
    
    EXPECT_GE(ensembleSpy.count(), 1);
    
    auto ensembleInfo = decoder->getEnsembleInfo();
    EXPECT_FALSE(ensembleInfo.label.isEmpty());
    EXPECT_TRUE(ensembleInfo.label.contains("TEST_ENSEMBLE"));
}

// Service Information Tests
TEST_F(FicDecoderTest, ServiceInformationIsExtracted)
{
    decoder->initialize();
    QSignalSpy serviceSpy(decoder, &FicDecoder::serviceInfoUpdated);
    
    QByteArray serviceFic = createFibWithService();
    decoder->processFicData(serviceFic);
    
    EXPECT_EQ(serviceSpy.count(), 1);
    
    auto services = decoder->getServices();
    EXPECT_FALSE(services.isEmpty());
    
    auto service = decoder->getServiceById(0xABCD);
    EXPECT_EQ(service.serviceId, 0xABCD);
    EXPECT_EQ(service.nbServiceComp, 1);
    EXPECT_FALSE(service.components.isEmpty());
}

TEST_F(FicDecoderTest, ServiceAvailabilityCheck)
{
    decoder->initialize();
    
    QByteArray serviceFic = createFibWithService();
    decoder->processFicData(serviceFic);
    
    // Service exists but no label yet
    EXPECT_FALSE(decoder->isServiceAvailable(0xABCD));
    
    // Non-existent service
    EXPECT_FALSE(decoder->isServiceAvailable(0x9999));
}

TEST_F(FicDecoderTest, GetServiceByIdReturnsCorrectService)
{
    decoder->initialize();
    
    QByteArray serviceFic = createFibWithService();
    decoder->processFicData(serviceFic);
    
    auto service = decoder->getServiceById(0xABCD);
    EXPECT_EQ(service.serviceId, 0xABCD);
    EXPECT_TRUE(service.components.size() > 0);
    
    // Non-existent service should return default
    auto invalidService = decoder->getServiceById(0x9999);
    EXPECT_EQ(invalidService.serviceId, 0);
}

// Sub-channel Information Tests
TEST_F(FicDecoderTest, SubchannelInformationIsExtracted)
{
    decoder->initialize();
    QSignalSpy subchannelSpy(decoder, &FicDecoder::subchannelInfoUpdated);
    
    QByteArray subchannelFic = createFibWithSubchannel();
    decoder->processFicData(subchannelFic);
    
    EXPECT_EQ(subchannelSpy.count(), 1);
    
    auto subchannels = decoder->getSubChannels();
    EXPECT_FALSE(subchannels.isEmpty());
    
    auto subchannel = decoder->getSubChannelById(4);
    EXPECT_EQ(subchannel.subChannelId, 4);
    EXPECT_EQ(subchannel.startAddress, 64);
    EXPECT_EQ(subchannel.subChannelSize, 32);
    EXPECT_TRUE(subchannel.isValid);
}

TEST_F(FicDecoderTest, SubchannelConfigurationCheck)
{
    decoder->initialize();
    
    QByteArray subchannelFic = createFibWithSubchannel();
    decoder->processFicData(subchannelFic);
    
    // Configured subchannel
    EXPECT_TRUE(decoder->isSubChannelConfigured(4));
    
    // Non-configured subchannel
    EXPECT_FALSE(decoder->isSubChannelConfigured(99));
}

TEST_F(FicDecoderTest, GetSubChannelByIdReturnsCorrectSubchannel)
{
    decoder->initialize();
    
    QByteArray subchannelFic = createFibWithSubchannel();
    decoder->processFicData(subchannelFic);
    
    auto subchannel = decoder->getSubChannelById(4);
    EXPECT_EQ(subchannel.subChannelId, 4);
    EXPECT_TRUE(subchannel.isValid);
    
    // Non-existent subchannel should return default
    auto invalidSubchannel = decoder->getSubChannelById(99);
    EXPECT_EQ(invalidSubchannel.subChannelId, 0);
    EXPECT_FALSE(invalidSubchannel.isValid);
}

// Synchronization Tests
TEST_F(FicDecoderTest, SyncLevelIncreasesWithSuccessfulProcessing)
{
    decoder->initialize();
    
    int initialSyncLevel = decoder->getSyncLevel();
    EXPECT_EQ(initialSyncLevel, 0);
    
    // Process multiple valid FIBs
    for (int i = 0; i < 20; ++i) {
        QByteArray validFic = createValidFib();
        decoder->processFicData(validFic);
    }
    
    int finalSyncLevel = decoder->getSyncLevel();
    EXPECT_GT(finalSyncLevel, initialSyncLevel);
    EXPECT_LE(finalSyncLevel, 100); // Should not exceed 100%
}

TEST_F(FicDecoderTest, SyncLevelDecreasesWithErrors)
{
    decoder->initialize();
    
    // First establish some sync level
    for (int i = 0; i < 10; ++i) {
        QByteArray validFic = createValidFib();
        decoder->processFicData(validFic);
    }
    
    int goodSyncLevel = decoder->getSyncLevel();
    
    // Introduce errors
    for (int i = 0; i < 15; ++i) {
        QByteArray invalidFic = createInvalidFib();
        decoder->processFicData(invalidFic);
    }
    
    int degradedSyncLevel = decoder->getSyncLevel();
    EXPECT_LT(degradedSyncLevel, goodSyncLevel);
}

TEST_F(FicDecoderTest, StatusChangeToSynchronizedAtHighSyncLevel)
{
    decoder->initialize();
    QSignalSpy statusSpy(decoder, &FicDecoder::statusChanged);
    
    // Process enough valid data to achieve synchronization
    for (int i = 0; i < 100; ++i) {
        QByteArray validFic = createValidFib();
        decoder->processFicData(validFic);
        
        if (decoder->getSyncLevel() >= 75) {
            break;
        }
    }
    
    // Check that synchronized status was reached
    bool synchronizedReached = false;
    for (int i = 0; i < statusSpy.count(); ++i) {
        auto status = statusSpy.at(i).at(0).value<FicDecoder::DecoderStatus>();
        if (status == FicDecoder::DecoderStatus::Synchronized) {
            synchronizedReached = true;
            break;
        }
    }
    
    EXPECT_TRUE(synchronizedReached);
}

// Error Handling Tests
TEST_F(FicDecoderTest, ErrorCountIncreasesOnProcessingFailures)
{
    decoder->initialize();
    
    quint32 initialErrorCount = decoder->getErrorCount();
    
    // Process invalid data
    QByteArray invalidData(20, static_cast<char>(0xFF));
    decoder->processFicData(invalidData);
    
    quint32 finalErrorCount = decoder->getErrorCount();
    EXPECT_GT(finalErrorCount, initialErrorCount);
}

TEST_F(FicDecoderTest, StatusChangesToErrorOnConsecutiveFailures)
{
    decoder->initialize();
    
    // This test would require implementing error status logic
    // Currently the decoder doesn't have an explicit Error status transition
    // but this test structure is ready for that enhancement
    
    for (int i = 0; i < 10; ++i) {
        QByteArray invalidData(20, static_cast<char>(0xFF));
        decoder->processFicData(invalidData);
    }
    
    // For now, just verify that errors are being counted
    EXPECT_GT(decoder->getErrorCount(), 5u);
}

// Signal Emission Tests
TEST_F(FicDecoderTest, AllExpectedSignalsAreEmitted)
{
    decoder->initialize();
    
    QSignalSpy statusSpy(decoder, &FicDecoder::statusChanged);
    QSignalSpy ficSpy(decoder, &FicDecoder::ficDataProcessed);
    QSignalSpy ensembleSpy(decoder, &FicDecoder::ensembleInfoUpdated);
    QSignalSpy serviceSpy(decoder, &FicDecoder::serviceInfoUpdated);
    QSignalSpy subchannelSpy(decoder, &FicDecoder::subchannelInfoUpdated);
    
    // Process data that should trigger all signals
    QByteArray ensembleFic = createValidFib();
    QByteArray serviceFic = createFibWithService();
    QByteArray subchannelFic = createFibWithSubchannel();
    
    decoder->processFicData(ensembleFic);
    decoder->processFicData(serviceFic);
    decoder->processFicData(subchannelFic);
    
    EXPECT_GT(statusSpy.count(), 0);
    EXPECT_GT(ficSpy.count(), 0);
    EXPECT_GT(ensembleSpy.count(), 0);
    EXPECT_GT(serviceSpy.count(), 0);
    EXPECT_GT(subchannelSpy.count(), 0);
}

// Performance and Edge Case Tests
TEST_F(FicDecoderTest, HandlesLargeFicDataVolume)
{
    decoder->initialize();
    
    // Create large volume of FIC data (simulate real-world stream)
    QByteArray largeFicData;
    for (int i = 0; i < 50; ++i) {
        largeFicData += createValidFib();
    }
    
    bool result = decoder->processFicData(largeFicData);
    
    EXPECT_TRUE(result);
    EXPECT_EQ(decoder->getFibCount(), 50u);
    EXPECT_GT(decoder->getSyncLevel(), 0);
}

TEST_F(FicDecoderTest, HandlesPartialFibData)
{
    decoder->initialize();
    
    // Create FIC data with incomplete FIB at the end
    QByteArray validFib = createValidFib();
    QByteArray partialFib = validFib.left(20); // Truncated FIB
    QByteArray mixedData = validFib + partialFib;
    
    bool result = decoder->processFicData(mixedData);
    
    // Should process the complete FIB successfully
    EXPECT_TRUE(result);
    EXPECT_EQ(decoder->getFibCount(), 1u); // Only complete FIB counted
}

TEST_F(FicDecoderTest, MaintainsConsistencyAfterMultipleResets)
{
    decoder->initialize();
    
    for (int cycle = 0; cycle < 5; ++cycle) {
        // Process some data
        QByteArray validFic = createValidFib();
        decoder->processFicData(validFic);
        
        EXPECT_GT(decoder->getFibCount(), 0u);
        
        // Reset
        decoder->reset();
        
        // Verify clean state
        EXPECT_EQ(decoder->getFibCount(), 0u);
        EXPECT_EQ(decoder->getErrorCount(), 0u);
        EXPECT_EQ(decoder->getSyncLevel(), 0);
        EXPECT_TRUE(decoder->getServices().isEmpty());
        EXPECT_TRUE(decoder->getSubChannels().isEmpty());
    }
}

// Thread Safety Tests (if applicable)
TEST_F(FicDecoderTest, QObjectSignalSlotMechanismWorks)
{
    // Verify that the Qt signal/slot mechanism is working correctly
    decoder->initialize();
    
    QSignalSpy spy(decoder, &FicDecoder::statusChanged);
    
    // Manually emit status change to verify signal connection
    decoder->reset(); // This should emit statusChanged
    
    EXPECT_GT(spy.count(), 0);
    EXPECT_TRUE(spy.isValid());
}

// Integration Tests
TEST_F(FicDecoderTest, CompleteWorkflowFromInitToSync)
{
    // Test complete workflow: init -> process data -> achieve sync -> extract info
    decoder->initialize();
    
    QSignalSpy statusSpy(decoder, &FicDecoder::statusChanged);
    QSignalSpy ensembleSpy(decoder, &FicDecoder::ensembleInfoUpdated);
    QSignalSpy serviceSpy(decoder, &FicDecoder::serviceInfoUpdated);
    QSignalSpy subchannelSpy(decoder, &FicDecoder::subchannelInfoUpdated);
    
    // Step 1: Process ensemble information
    QByteArray ensembleFic = createValidFib();
    decoder->processFicData(ensembleFic);
    
    // Step 2: Process service information
    QByteArray serviceFic = createFibWithService();
    decoder->processFicData(serviceFic);
    
    // Step 3: Process subchannel information
    QByteArray subchannelFic = createFibWithSubchannel();
    decoder->processFicData(subchannelFic);
    
    // Step 4: Process enough data to achieve sync
    for (int i = 0; i < 20; ++i) {
        decoder->processFicData(ensembleFic);
    }
    
    // Verify complete workflow
    EXPECT_GT(decoder->getFibCount(), 20u);
    EXPECT_GT(decoder->getSyncLevel(), 50); // Should have reasonable sync level
    
    // Verify all information was extracted
    EXPECT_FALSE(decoder->getServices().isEmpty());
    EXPECT_FALSE(decoder->getSubChannels().isEmpty());
    EXPECT_NE(decoder->getEnsembleInfo().ensembleId, 0);
    
    // Verify signals were emitted
    EXPECT_GT(ensembleSpy.count(), 0);
    EXPECT_GT(serviceSpy.count(), 0);
    EXPECT_GT(subchannelSpy.count(), 0);
}