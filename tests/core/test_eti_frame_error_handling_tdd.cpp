/**
 * @file test_eti_frame_error_handling_tdd.cpp
 * @brief TDD/AAA tests for ETI frame error handling
 * 
 * Following proper Test-Driven Development with Arrange-Act-Assert pattern
 */

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <QSignalSpy>
#include <QTest>

#include "core/eti_processor.hpp"
#include "utils/logger.h"

using ::testing::_;
using ::testing::Return;
using ::testing::InSequence;

class EtiFrameErrorHandlingTDD : public ::testing::Test 
{
protected:
    void SetUp() override 
    {
        // ARRANGE: Set up test environment
        m_processor = std::make_unique<EtiProcessor>();
        m_processor->initialize();
        
        // Set up signal spies for verification
        m_frameErrorSpy = std::make_unique<QSignalSpy>(m_processor.get(), 
                                                       &EtiProcessor::frameError);
        m_frameRecoveredSpy = std::make_unique<QSignalSpy>(m_processor.get(), 
                                                           &EtiProcessor::frameRecovered);
        m_errorStatsSpy = std::make_unique<QSignalSpy>(m_processor.get(), 
                                                       &EtiProcessor::errorStatisticsUpdated);
        
        Logger::instance().setLogLevel(Logger::Warning); // Reduce test noise
    }

    void TearDown() override 
    {
        m_processor.reset();
    }

    // Helper methods for creating test frames
    QByteArray createValidETIFrame() 
    {
        QByteArray frame(6144, 0x00);
        // Add valid sync pattern
        frame[0] = 0x49;
        frame[1] = 0x93; 
        frame[2] = 0x1E;
        frame[3] = 0x03;
        return frame;
    }

    QByteArray createSyncErrorFrame() 
    {
        QByteArray frame = createValidETIFrame();
        // Corrupt sync pattern
        frame[0] = 0x12; // Wrong byte
        return frame;
    }

    QByteArray createSizeErrorFrame() 
    {
        // Create frame with wrong size
        return QByteArray(1000, 0x00); // Too small
    }

    QByteArray createCRCErrorFrame() 
    {
        QByteArray frame = createValidETIFrame();
        // Corrupt CRC at end (last 2 bytes)
        frame[6142] = 0xFF;
        frame[6143] = 0xFF;
        return frame;
    }

protected:
    std::unique_ptr<EtiProcessor> m_processor;
    std::unique_ptr<QSignalSpy> m_frameErrorSpy;
    std::unique_ptr<QSignalSpy> m_frameRecoveredSpy;
    std::unique_ptr<QSignalSpy> m_errorStatsSpy;
};

// 🔴 RED PHASE TESTS - These should FAIL initially
TEST_F(EtiFrameErrorHandlingTDD, ShouldDetectFrameSizeError_RED)
{
    // ARRANGE
    QByteArray tooSmallFrame = createSizeErrorFrame();
    
    // ACT
    bool result = m_processor->processEtiFrame(tooSmallFrame);
    
    // ASSERT
    EXPECT_FALSE(result) << "Should reject frames with wrong size";
    
    // Verify error signal was emitted
    ASSERT_EQ(1, m_frameErrorSpy->count()) << "Should emit frameError signal";
    QList<QVariant> errorArgs = m_frameErrorSpy->takeFirst();
    EXPECT_EQ("SIZE_ERROR", errorArgs.at(1).toString()) << "Should report SIZE_ERROR";
    EXPECT_EQ("Critical", errorArgs.at(3).toString()) << "Size errors should be Critical severity";
    
    // Verify error statistics
    auto stats = m_processor->getErrorStatistics();
    EXPECT_EQ(1, stats.sizeErrors) << "Should increment size error count";
    EXPECT_EQ(1, stats.totalFrames) << "Should increment total frame count";
    EXPECT_EQ(0, stats.validFrames) << "Should not increment valid frame count";
}

TEST_F(EtiFrameErrorHandlingTDD, ShouldDetectSyncPatternError_RED)
{
    // ARRANGE
    QByteArray corruptSyncFrame = createSyncErrorFrame();
    
    // ACT
    bool result = m_processor->processEtiFrame(corruptSyncFrame);
    
    // ASSERT
    EXPECT_FALSE(result) << "Should reject frames with corrupt sync pattern";
    
    // Verify error signal was emitted
    ASSERT_EQ(1, m_frameErrorSpy->count()) << "Should emit frameError signal";
    QList<QVariant> errorArgs = m_frameErrorSpy->takeFirst();
    EXPECT_EQ("SYNC_ERROR", errorArgs.at(1).toString()) << "Should report SYNC_ERROR";
    EXPECT_EQ("High", errorArgs.at(3).toString()) << "Sync errors should be High severity";
    
    // Verify error statistics
    auto stats = m_processor->getErrorStatistics();
    EXPECT_EQ(1, stats.syncErrors) << "Should increment sync error count";
}

TEST_F(EtiFrameErrorHandlingTDD, ShouldDetectCRCError_RED)
{
    // ARRANGE
    QByteArray corruptCRCFrame = createCRCErrorFrame();
    
    // ACT
    bool result = m_processor->processEtiFrame(corruptCRCFrame);
    
    // ASSERT  
    EXPECT_TRUE(result) << "Should continue processing despite CRC error (data may still be usable)";
    
    // Verify error signal was emitted
    ASSERT_EQ(1, m_frameErrorSpy->count()) << "Should emit frameError signal";
    QList<QVariant> errorArgs = m_frameErrorSpy->takeFirst();
    EXPECT_EQ("CRC_ERROR", errorArgs.at(1).toString()) << "Should report CRC_ERROR";
    EXPECT_EQ("Medium", errorArgs.at(3).toString()) << "CRC errors should be Medium severity";
    
    // Verify error statistics
    auto stats = m_processor->getErrorStatistics();
    EXPECT_EQ(1, stats.crcErrors) << "Should increment CRC error count";
}

TEST_F(EtiFrameErrorHandlingTDD, ShouldRecoverFromSyncError_RED)
{
    // ARRANGE
    QByteArray corruptSyncFrame = createSyncErrorFrame();
    
    // ACT
    bool result = m_processor->processEtiFrame(corruptSyncFrame);
    
    // ASSERT
    EXPECT_TRUE(result) << "Should recover from sync errors";
    
    // Verify recovery signal was emitted
    ASSERT_EQ(1, m_frameRecoveredSpy->count()) << "Should emit frameRecovered signal";
    QList<QVariant> recoveryArgs = m_frameRecoveredSpy->takeFirst();
    EXPECT_EQ("Sync pattern correction", recoveryArgs.at(1).toString()) << "Should report sync recovery method";
    
    // Verify recovery statistics
    auto stats = m_processor->getErrorStatistics();
    EXPECT_EQ(1, stats.recoveredFrames) << "Should increment recovered frame count";
}

TEST_F(EtiFrameErrorHandlingTDD, ShouldProcessValidFrameWithoutErrors_RED)
{
    // ARRANGE
    QByteArray validFrame = createValidETIFrame();
    
    // ACT
    bool result = m_processor->processEtiFrame(validFrame);
    
    // ASSERT
    EXPECT_TRUE(result) << "Should successfully process valid frames";
    
    // Verify no error signals were emitted
    EXPECT_EQ(0, m_frameErrorSpy->count()) << "Should not emit frameError for valid frames";
    EXPECT_EQ(0, m_frameRecoveredSpy->count()) << "Should not emit frameRecovered for valid frames";
    
    // Verify statistics updated correctly
    ASSERT_EQ(1, m_errorStatsSpy->count()) << "Should emit errorStatisticsUpdated signal";
    
    auto stats = m_processor->getErrorStatistics();
    EXPECT_EQ(1, stats.totalFrames) << "Should increment total frame count";
    EXPECT_EQ(1, stats.validFrames) << "Should increment valid frame count";
    EXPECT_EQ(0.0, stats.errorRate) << "Should have 0% error rate for valid frames";
}

TEST_F(EtiFrameErrorHandlingTDD, ShouldCalculateCorrectErrorRate_RED)
{
    // ARRANGE
    QByteArray validFrame = createValidETIFrame();
    QByteArray errorFrame = createSizeErrorFrame();
    
    // ACT
    m_processor->processEtiFrame(validFrame);   // 1 valid
    m_processor->processEtiFrame(errorFrame);   // 1 error
    m_processor->processEtiFrame(validFrame);   // 2 valid
    m_processor->processEtiFrame(errorFrame);   // 2 errors
    
    // ASSERT
    auto stats = m_processor->getErrorStatistics();
    EXPECT_EQ(4, stats.totalFrames) << "Should have processed 4 frames total";
    EXPECT_EQ(2, stats.validFrames) << "Should have 2 valid frames";
    EXPECT_DOUBLE_EQ(50.0, stats.errorRate) << "Should calculate 50% error rate";
}

TEST_F(EtiFrameErrorHandlingTDD, ShouldResetErrorStatistics_RED)
{
    // ARRANGE
    QByteArray errorFrame = createSizeErrorFrame();
    m_processor->processEtiFrame(errorFrame); // Generate some errors
    
    // ACT
    m_processor->resetErrorStatistics();
    
    // ASSERT
    auto stats = m_processor->getErrorStatistics();
    EXPECT_EQ(0, stats.totalFrames) << "Should reset total frame count";
    EXPECT_EQ(0, stats.validFrames) << "Should reset valid frame count";
    EXPECT_EQ(0, stats.sizeErrors) << "Should reset size error count";
    EXPECT_EQ(0.0, stats.errorRate) << "Should reset error rate";
    
    // Verify reset signal was emitted
    ASSERT_EQ(1, m_errorStatsSpy->count()) << "Should emit errorStatisticsUpdated after reset";
}

// Multi-frame error handling tests
TEST_F(EtiFrameErrorHandlingTDD, ShouldHandleMultipleErrorTypes_RED)
{
    // ARRANGE
    QByteArray sizeError = createSizeErrorFrame();
    QByteArray syncError = createSyncErrorFrame(); 
    QByteArray crcError = createCRCErrorFrame();
    QByteArray validFrame = createValidETIFrame();
    
    // ACT
    m_processor->processEtiFrame(sizeError);
    m_processor->processEtiFrame(syncError);
    m_processor->processEtiFrame(crcError);
    m_processor->processEtiFrame(validFrame);
    
    // ASSERT
    auto stats = m_processor->getErrorStatistics();
    EXPECT_EQ(4, stats.totalFrames) << "Should count all processed frames";
    EXPECT_EQ(1, stats.sizeErrors) << "Should count size errors";
    EXPECT_EQ(1, stats.syncErrors) << "Should count sync errors";  
    EXPECT_EQ(1, stats.crcErrors) << "Should count CRC errors";
    EXPECT_GT(stats.recoveredFrames, 0) << "Should have some recovered frames";
    
    // Should have emitted error signals for each error type
    EXPECT_EQ(3, m_frameErrorSpy->count()) << "Should emit 3 error signals";
}

// Performance test - error handling should not significantly impact processing speed
TEST_F(EtiFrameErrorHandlingTDD, ShouldMaintainPerformanceWithErrors_RED)
{
    // ARRANGE
    const int FRAME_COUNT = 1000;
    QByteArray validFrame = createValidETIFrame();
    QByteArray errorFrame = createSyncErrorFrame(); // Recoverable error
    
    // ACT
    QElapsedTimer timer;
    timer.start();
    
    for (int i = 0; i < FRAME_COUNT; ++i) {
        // Mix of valid and error frames
        if (i % 10 == 0) {
            m_processor->processEtiFrame(errorFrame); // 10% error rate
        } else {
            m_processor->processEtiFrame(validFrame);
        }
    }
    
    qint64 elapsedMs = timer.elapsed();
    
    // ASSERT
    double framesPerSecond = (FRAME_COUNT * 1000.0) / elapsedMs;
    EXPECT_GT(framesPerSecond, 1000.0) << "Should maintain >1000 FPS even with 10% error rate";
    
    auto stats = m_processor->getErrorStatistics();
    EXPECT_EQ(FRAME_COUNT, stats.totalFrames) << "Should process all frames";
    EXPECT_DOUBLE_EQ(10.0, stats.errorRate) << "Should correctly calculate 10% error rate";
}