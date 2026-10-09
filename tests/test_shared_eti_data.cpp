// tests/test_shared_eti_data.cpp
#include <gtest/gtest.h>
#include <QCoreApplication>
#include <QSignalSpy>
#include <QThread>
#include <QDateTime>
#include "../src/core/shared_eti_data.hpp"

using namespace streamdab::core;

class SharedETIDataTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Create Qt application for signal/slot
        int argc = 0;
        char** argv = nullptr;
        if (!QCoreApplication::instance()) {
            app = new QCoreApplication(argc, argv);
        }
        
        sharedData = new SharedETIData();
    }
    
    void TearDown() override {
        delete sharedData;
        sharedData = nullptr;
    }
    
    QCoreApplication* app = nullptr;
    SharedETIData* sharedData = nullptr;
};

// ============================================================================
// ETI Frame Operations
// ============================================================================

TEST_F(SharedETIDataTest, SetAndGetCurrentFrame) {
    ETIFrame frame;
    frame.err = 0xFF;
    frame.fct = 42;
    frame.nst = 5;
    frame.frame_number = 1234;
    frame.crc_valid = true;
    frame.raw_data = QByteArray(6144, 0x00);
    
    sharedData->setCurrentFrame(frame);
    
    ETIFrame retrieved = sharedData->getCurrentFrame();
    
    EXPECT_EQ(retrieved.err, 0xFF);
    EXPECT_EQ(retrieved.fct, 42);
    EXPECT_EQ(retrieved.nst, 5);
    EXPECT_EQ(retrieved.frame_number, 1234);
    EXPECT_TRUE(retrieved.crc_valid);
    EXPECT_EQ(retrieved.raw_data.size(), 6144);
}

TEST_F(SharedETIDataTest, FrameUpdatedSignal) {
    QSignalSpy spy(sharedData, &SharedETIData::frameUpdated);
    
    ETIFrame frame;
    frame.frame_number = 999;
    
    sharedData->setCurrentFrame(frame);
    
    EXPECT_EQ(spy.count(), 1);
    
    QList<QVariant> arguments = spy.takeFirst();
    EXPECT_EQ(arguments.at(0).toULongLong(), 999);
}

TEST_F(SharedETIDataTest, MultipleFrameUpdates) {
    QSignalSpy spy(sharedData, &SharedETIData::frameUpdated);
    
    for (uint64_t i = 1; i <= 10; ++i) {
        ETIFrame frame;
        frame.frame_number = i;
        sharedData->setCurrentFrame(frame);
    }
    
    EXPECT_EQ(spy.count(), 10);
    
    ETIFrame last = sharedData->getCurrentFrame();
    EXPECT_EQ(last.frame_number, 10);
}

// ============================================================================
// FIC Data Operations
// ============================================================================

TEST_F(SharedETIDataTest, SetAndGetFICData) {
    FICData fic;
    fic.fic_content = QByteArray(96, 0xAA);
    fic.ensemble_id = 0x1234;
    fic.ensemble_label = "Test Ensemble";
    
    sharedData->setFICData(fic);
    
    FICData retrieved = sharedData->getFICData();
    
    EXPECT_EQ(retrieved.fic_content.size(), 96);
    EXPECT_EQ(retrieved.ensemble_id, 0x1234);
    EXPECT_EQ(retrieved.ensemble_label, "Test Ensemble");
}

TEST_F(SharedETIDataTest, FICDataUpdatedSignal) {
    QSignalSpy spy(sharedData, &SharedETIData::ficDataUpdated);
    
    FICData fic;
    fic.ensemble_id = 0x5678;
    
    sharedData->setFICData(fic);
    
    EXPECT_EQ(spy.count(), 1);
}

// ============================================================================
// Service Management
// ============================================================================

TEST_F(SharedETIDataTest, UpdateAndGetService) {
    ServiceInfo service;
    service.service_id = 0xABCD;
    service.service_label = "Test Service";
    service.subchannel_id = 3;
    service.is_audio = true;
    service.is_dabplus = true;
    service.bitrate = 128;
    
    sharedData->updateService(service);
    
    ServiceInfo retrieved = sharedData->getService(0xABCD);
    
    EXPECT_EQ(retrieved.service_id, 0xABCD);
    EXPECT_EQ(retrieved.service_label, "Test Service");
    EXPECT_EQ(retrieved.subchannel_id, 3);
    EXPECT_TRUE(retrieved.is_audio);
    EXPECT_TRUE(retrieved.is_dabplus);
    EXPECT_EQ(retrieved.bitrate, 128);
}

TEST_F(SharedETIDataTest, ServiceListUpdatedSignal) {
    QSignalSpy spy(sharedData, &SharedETIData::serviceListUpdated);
    
    ServiceInfo service;
    service.service_id = 0x1111;
    
    sharedData->updateService(service);
    
    EXPECT_EQ(spy.count(), 1);
    
    QList<QVariant> arguments = spy.takeFirst();
    EXPECT_EQ(arguments.at(0).toInt(), 1);
}

TEST_F(SharedETIDataTest, ServiceAddedSignal) {
    QSignalSpy spy(sharedData, &SharedETIData::serviceAdded);
    
    ServiceInfo service;
    service.service_id = 0x2222;
    service.service_label = "New Service";
    
    sharedData->updateService(service);
    
    EXPECT_EQ(spy.count(), 1);
    
    QList<QVariant> arguments = spy.takeFirst();
    EXPECT_EQ(arguments.at(0).toUInt(), 0x2222);
    EXPECT_EQ(arguments.at(1).toString(), "New Service");
}

TEST_F(SharedETIDataTest, MultipleServicesManagement) {
    for (uint32_t i = 1; i <= 5; ++i) {
        ServiceInfo service;
        service.service_id = i;
        service.service_label = QString("Service %1").arg(i);
        sharedData->updateService(service);
    }
    
    EXPECT_EQ(sharedData->getServiceCount(), 5);
    
    QMap<uint32_t, ServiceInfo> services = sharedData->getServices();
    EXPECT_EQ(services.size(), 5);
    
    EXPECT_TRUE(services.contains(1));
    EXPECT_TRUE(services.contains(5));
}

TEST_F(SharedETIDataTest, ClearServices) {
    // Add services
    for (uint32_t i = 1; i <= 3; ++i) {
        ServiceInfo service;
        service.service_id = i;
        sharedData->updateService(service);
    }
    
    EXPECT_EQ(sharedData->getServiceCount(), 3);
    
    QSignalSpy spy(sharedData, &SharedETIData::serviceListUpdated);
    
    // Clear services
    sharedData->clearServices();
    
    EXPECT_EQ(sharedData->getServiceCount(), 0);
    EXPECT_EQ(spy.count(), 1);
}

// ============================================================================
// Subchannel Configuration
// ============================================================================

TEST_F(SharedETIDataTest, SetAndGetSubchannelConfig) {
    SubchannelConfig config;
    config.subchannel_id = 5;
    config.start_address = 100;
    config.size = 84;
    config.protection_level = 3;
    config.is_uep = true;
    
    sharedData->setSubchannelConfig(5, config);
    
    QMap<uint8_t, SubchannelConfig> subchannels = sharedData->getSubchannels();
    
    EXPECT_EQ(subchannels.size(), 1);
    EXPECT_TRUE(subchannels.contains(5));
    
    SubchannelConfig retrieved = subchannels[5];
    EXPECT_EQ(retrieved.start_address, 100);
    EXPECT_EQ(retrieved.size, 84);
    EXPECT_EQ(retrieved.protection_level, 3);
    EXPECT_TRUE(retrieved.is_uep);
}

TEST_F(SharedETIDataTest, MultipleSubchannels) {
    for (uint8_t i = 0; i < 10; ++i) {
        SubchannelConfig config;
        config.subchannel_id = i;
        config.start_address = i * 100;
        sharedData->setSubchannelConfig(i, config);
    }
    
    EXPECT_EQ(sharedData->getSubchannelCount(), 10);
}

// ============================================================================
// MSC Data Zero-Copy Operations
// ============================================================================

TEST_F(SharedETIDataTest, SetAndGetMSCData) {
    QSharedPointer<QByteArray> data = QSharedPointer<QByteArray>::create(1024, 0xBB);
    
    sharedData->setMSCData(3, data);
    
    QSharedPointer<QByteArray> retrieved = sharedData->getMSCData(3);
    
    ASSERT_FALSE(retrieved.isNull());
    EXPECT_EQ(retrieved->size(), 1024);
    EXPECT_EQ(retrieved->at(0), static_cast<char>(0xBB));
}

TEST_F(SharedETIDataTest, MSCDataUpdatedSignal) {
    QSignalSpy spy(sharedData, &SharedETIData::mscDataUpdated);
    
    QSharedPointer<QByteArray> data = QSharedPointer<QByteArray>::create(512, 0xCC);
    
    sharedData->setMSCData(7, data);
    
    EXPECT_EQ(spy.count(), 1);
    
    QList<QVariant> arguments = spy.takeFirst();
    EXPECT_EQ(arguments.at(0).toUInt(), 7);
}

TEST_F(SharedETIDataTest, MSCDataZeroCopy) {
    // Create shared data
    QSharedPointer<QByteArray> original = QSharedPointer<QByteArray>::create(2048, 0xDD);
    
    // Store in shared data model
    sharedData->setMSCData(10, original);
    
    // Retrieve (should be same shared pointer)
    QSharedPointer<QByteArray> retrieved = sharedData->getMSCData(10);
    
    ASSERT_FALSE(retrieved.isNull());
    
    // Verify zero-copy (same data pointer)
    EXPECT_EQ(original.data(), retrieved.data());
}

// ============================================================================
// Error Statistics
// ============================================================================

TEST_F(SharedETIDataTest, UpdateErrorStats) {
    ErrorStats stats;
    stats.total_frames = 1000;
    stats.crc_errors = 5;
    stats.sync_errors = 2;
    stats.fic_errors = 1;
    stats.avg_processing_time_ms = 2.5;
    stats.max_processing_time_ms = 10.0;
    
    sharedData->updateErrorStats(stats);
    
    ErrorStats retrieved = sharedData->getErrorStats();
    
    EXPECT_EQ(retrieved.total_frames, 1000);
    EXPECT_EQ(retrieved.crc_errors, 5);
    EXPECT_EQ(retrieved.sync_errors, 2);
    EXPECT_EQ(retrieved.fic_errors, 1);
    EXPECT_DOUBLE_EQ(retrieved.avg_processing_time_ms, 2.5);
    EXPECT_DOUBLE_EQ(retrieved.max_processing_time_ms, 10.0);
}

TEST_F(SharedETIDataTest, ErrorStatsUpdatedSignal) {
    QSignalSpy spy(sharedData, &SharedETIData::errorStatsUpdated);
    
    ErrorStats stats;
    stats.total_frames = 500;
    
    sharedData->updateErrorStats(stats);
    
    EXPECT_EQ(spy.count(), 1);
}

TEST_F(SharedETIDataTest, IncrementErrorCount) {
    QSignalSpy spy(sharedData, &SharedETIData::errorStatsUpdated);
    
    // Increment different error types
    sharedData->incrementErrorCount("crc");
    sharedData->incrementErrorCount("crc");
    sharedData->incrementErrorCount("sync");
    sharedData->incrementErrorCount("fic");
    
    EXPECT_EQ(spy.count(), 4);
    
    ErrorStats stats = sharedData->getErrorStats();
    EXPECT_EQ(stats.crc_errors, 2);
    EXPECT_EQ(stats.sync_errors, 1);
    EXPECT_EQ(stats.fic_errors, 1);
}

TEST_F(SharedETIDataTest, ResetErrorStats) {
    // Set some errors
    ErrorStats stats;
    stats.total_frames = 1000;
    stats.crc_errors = 10;
    sharedData->updateErrorStats(stats);
    
    // Reset
    sharedData->resetErrorStats();
    
    ErrorStats reset = sharedData->getErrorStats();
    EXPECT_EQ(reset.total_frames, 0);
    EXPECT_EQ(reset.crc_errors, 0);
}

// ============================================================================
// Utility Methods
// ============================================================================

TEST_F(SharedETIDataTest, ClearAll) {
    // Add various data
    ETIFrame frame;
    frame.frame_number = 100;
    sharedData->setCurrentFrame(frame);
    
    ServiceInfo service;
    service.service_id = 1;
    sharedData->updateService(service);
    
    SubchannelConfig config;
    config.subchannel_id = 5;
    sharedData->setSubchannelConfig(5, config);
    
    // Clear all
    sharedData->clearAll();
    
    // Verify everything is cleared
    EXPECT_EQ(sharedData->getCurrentFrame().frame_number, 0);
    EXPECT_EQ(sharedData->getServiceCount(), 0);
    EXPECT_EQ(sharedData->getSubchannelCount(), 0);
}

TEST_F(SharedETIDataTest, LastUpdateTime) {
    QDateTime before = QDateTime::currentDateTime();
    
    ETIFrame frame;
    sharedData->setCurrentFrame(frame);
    
    QDateTime lastUpdate = sharedData->getLastUpdateTime();
    
    QDateTime after = QDateTime::currentDateTime();
    
    EXPECT_TRUE(lastUpdate >= before);
    EXPECT_TRUE(lastUpdate <= after);
}

// ============================================================================
// Thread Safety Tests
// ============================================================================

TEST_F(SharedETIDataTest, ConcurrentReads) {
    // Setup initial data
    ETIFrame frame;
    frame.frame_number = 999;
    sharedData->setCurrentFrame(frame);
    
    // Create multiple reader threads
    QList<QThread*> threads;
    std::atomic<int> read_count(0);
    
    for (int i = 0; i < 5; ++i) {
        QThread* thread = QThread::create([this, &read_count]() {
            for (int j = 0; j < 100; ++j) {
                ETIFrame f = sharedData->getCurrentFrame();
                if (f.frame_number == 999) {
                    read_count++;
                }
            }
        });
        threads.append(thread);
        thread->start();
    }
    
    // Wait for all threads
    for (QThread* thread : threads) {
        thread->wait();
        delete thread;
    }
    
    // All reads should have succeeded
    EXPECT_EQ(read_count, 500);
}

TEST_F(SharedETIDataTest, ConcurrentWrites) {
    QSignalSpy spy(sharedData, &SharedETIData::frameUpdated);
    
    // Create multiple writer threads
    QList<QThread*> threads;
    
    for (int i = 0; i < 5; ++i) {
        QThread* thread = QThread::create([this, i]() {
            for (int j = 0; j < 20; ++j) {
                ETIFrame frame;
                frame.frame_number = i * 1000 + j;
                sharedData->setCurrentFrame(frame);
            }
        });
        threads.append(thread);
        thread->start();
    }
    
    // Wait for all threads
    for (QThread* thread : threads) {
        thread->wait();
        delete thread;
    }
    
    // All writes should have triggered signals
    EXPECT_EQ(spy.count(), 100);
}

// ============================================================================
// Ensemble Info Change Signal
// ============================================================================

TEST_F(SharedETIDataTest, EnsembleInfoChangedSignal) {
    QSignalSpy spy(sharedData, &SharedETIData::ensembleInfoChanged);
    
    FICData fic1;
    fic1.ensemble_id = 0x1111;
    fic1.ensemble_label = "Ensemble 1";
    sharedData->setFICData(fic1);
    
    EXPECT_EQ(spy.count(), 1);
    
    // Same ensemble ID - no signal
    FICData fic2;
    fic2.ensemble_id = 0x1111;
    fic2.ensemble_label = "Ensemble 1 Updated";
    sharedData->setFICData(fic2);
    
    EXPECT_EQ(spy.count(), 1);  // Still 1, not 2
    
    // Different ensemble ID - signal emitted
    FICData fic3;
    fic3.ensemble_id = 0x2222;
    fic3.ensemble_label = "Ensemble 2";
    sharedData->setFICData(fic3);
    
    EXPECT_EQ(spy.count(), 2);
}

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
