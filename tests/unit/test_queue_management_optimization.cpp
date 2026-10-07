/**
 * @file test_queue_management_optimization.cpp
 * @brief Queue Management Optimization Validation Tests
 * 
 * Comprehensive test suite for validating queue management optimizations
 * that prevent violations and maintain 100% ETSI compliance under high load.
 * 
 * @author TDD Agent
 * @date 2025
 * @copyright Copyright (c) 2025 ETI Stream Analyser Team
 * @version 1.0
 */

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <chrono>
#include <thread>
#include <vector>
#include <memory>
#include <atomic>
#include <future>
#include <queue>
#include <mutex>

// Core system headers
#include "../../src/core/etsi/realtime_monitor.h"
#include "../../src/core/etsi/compliance_engine.h"
#include "../../src/core/etsi/alert_system.h"

// Test fixtures
#include "../fixtures/eti_streams/optimized_compliance_frames.h"

using namespace etsi::realtime;
using namespace etsi::compliance;
using namespace etsi::alerts;

using ::testing::_;
using ::testing::Return;
using ::testing::InSequence;
using ::testing::StrictMock;

/**
 * @brief Test fixture for queue management optimization validation
 */
class QueueManagementOptimizationTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Configure optimized real-time monitor
        RealtimeMonitorConfig config;
        config.enable_urgent_processing = true;
        config.queue_optimization = true;
        config.max_queue_size = 1000;
        config.queue_overflow_strategy = QueueOverflowStrategy::PRIORITIZED_DROP;
        config.priority_boost_threshold = 950; // Boost priority when queue 95% full
        config.processing_thread_count = 4;
        config.enable_load_balancing = true;
        config.enable_adaptive_processing = true;
        config.max_processing_latency = std::chrono::milliseconds(50);
        
        monitor_ = std::make_unique<EtsiRealtimeMonitor>(config);
        ASSERT_TRUE(monitor_->initialize());
        
        // Configure compliance engine for real-time operation
        EtsiComplianceEngine::Config compliance_config;
        compliance_config.target_level = ComplianceLevel::BROADCAST_QUALITY;
        compliance_config.enable_fast_validation = true;
        compliance_config.enable_optimized_penalty_calculation = true;
        compliance_config.validation_timeout = std::chrono::milliseconds(10);
        
        compliance_engine_ = std::make_unique<EtsiComplianceEngine>(compliance_config);
        ASSERT_TRUE(compliance_engine_->initialize());
        
        // Configure alert system
        AlertSystemConfig alert_config;
        alert_config.enable_queue_monitoring = true;
        alert_config.queue_warning_threshold = 800;  // 80% full
        alert_config.queue_critical_threshold = 950; // 95% full
        alert_config.enable_performance_alerts = true;
        
        alert_system_ = std::make_unique<EtsiAlertSystem>(alert_config);
        ASSERT_TRUE(alert_system_->initialize());
        
        // Generate test data
        test_frames_ = fixtures::eti::CreateOptimizedComplianceFrames(2000);
        
        test_start_time_ = std::chrono::high_resolution_clock::now();
    }
    
    void TearDown() override {
        if (monitor_) {
            monitor_->stop();
        }
        
        auto test_end_time = std::chrono::high_resolution_clock::now();
        auto test_duration = std::chrono::duration_cast<std::chrono::milliseconds>(
            test_end_time - test_start_time_);
        
        std::cout << "Queue test completed in " << test_duration.count() << " ms" << std::endl;
        
        // Print queue statistics
        if (monitor_) {
            auto queue_stats = monitor_->get_queue_statistics();
            std::cout << "Queue Statistics:" << std::endl;
            std::cout << "  Max queue size: " << queue_stats.max_queue_size << std::endl;
            std::cout << "  Peak utilization: " << queue_stats.peak_utilization_percent << "%" << std::endl;
            std::cout << "  Dropped frames: " << queue_stats.dropped_frames << std::endl;
            std::cout << "  Processing rate: " << queue_stats.average_processing_rate << " fps" << std::endl;
        }
    }
    
    std::unique_ptr<EtsiRealtimeMonitor> monitor_;
    std::unique_ptr<EtsiComplianceEngine> compliance_engine_;
    std::unique_ptr<EtsiAlertSystem> alert_system_;
    std::vector<EtiFrame> test_frames_;
    std::chrono::high_resolution_clock::time_point test_start_time_;
};

/**
 * @brief Test basic queue management under normal load
 */
TEST_F(QueueManagementOptimizationTest, testNormalLoadQueueManagement) {
    ASSERT_TRUE(monitor_->start());
    
    uint32_t stream_id = 1;
    std::vector<ComplianceResult> results;
    
    // Process frames at normal rate
    for (size_t i = 0; i < 100; ++i) {
        auto frame_data = test_frames_[i].to_byte_vector();
        auto result = monitor_->process_eti_frame(stream_id, frame_data);
        results.push_back(result);
        
        // Normal processing interval (24ms for real-time DAB)
        std::this_thread::sleep_for(std::chrono::milliseconds(24));
    }
    
    // VALIDATE: No queue violations under normal load
    for (const auto& result : results) {
        auto violations = result.get_violations();
        for (const auto& violation : violations) {
            EXPECT_FALSE(violation.message.find("queue") != std::string::npos && 
                        violation.severity >= ValidationSeverity::WARNING)
                << "Queue violation detected under normal load: " << violation.message;
        }
    }
    
    // VALIDATE: Processing performance maintained
    auto stats = monitor_->get_processing_stats();
    EXPECT_LT(stats.average_latency_ms, 25.0); // <25ms latency
    EXPECT_GT(stats.frame_processing_rate, 35.0); // >35 FPS (normal DAB rate)
    
    // VALIDATE: Queue utilization reasonable
    auto queue_stats = monitor_->get_queue_statistics();
    EXPECT_LT(queue_stats.peak_utilization_percent, 50.0); // <50% utilization
}

/**
 * @brief Test queue management under high load stress
 */
TEST_F(QueueManagementOptimizationTest, testHighLoadStressTest) {
    ASSERT_TRUE(monitor_->start());
    
    uint32_t stream_id = 1;
    std::atomic<size_t> frames_processed(0);
    std::atomic<size_t> violations_detected(0);
    
    // High load stress test - burst processing
    auto start_time = std::chrono::high_resolution_clock::now();
    
    // Submit frames rapidly to stress the queue
    std::vector<std::future<ComplianceResult>> futures;
    for (size_t i = 0; i < 2000; ++i) {
        auto frame_data = test_frames_[i % test_frames_.size()].to_byte_vector();
        
        // Submit frame for processing
        auto future = std::async(std::launch::async, [&, stream_id, frame_data]() {
            auto result = monitor_->process_eti_frame(stream_id, frame_data);
            
            // Check for queue violations
            auto violations = result.get_violations();
            for (const auto& violation : violations) {
                if (violation.message.find("queue") != std::string::npos && 
                    violation.severity >= ValidationSeverity::WARNING) {
                    violations_detected.fetch_add(1);
                }
            }
            
            frames_processed.fetch_add(1);
            return result;
        });
        
        futures.push_back(std::move(future));
        
        // Brief pause to prevent overwhelming system
        if (i % 100 == 0) {
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
    }
    
    // Wait for all processing to complete
    std::vector<ComplianceResult> results;
    for (auto& future : futures) {
        results.push_back(future.get());
    }
    
    auto end_time = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
    
    // VALIDATE: Queue optimization prevented violations
    EXPECT_EQ(violations_detected.load(), 0) 
        << "Queue violations detected under high load stress";
    
    // VALIDATE: All frames processed
    EXPECT_EQ(frames_processed.load(), 2000);
    
    // VALIDATE: Performance maintained under stress
    double fps = 2000.0 / (duration.count() / 1000.0);
    EXPECT_GT(fps, 1000.0); // >1000 FPS under stress
    
    // VALIDATE: Queue handled the load effectively
    auto queue_stats = monitor_->get_queue_statistics();
    EXPECT_LT(queue_stats.dropped_frames, 10); // <10 dropped frames acceptable
    EXPECT_LT(queue_stats.peak_utilization_percent, 98.0); // Should not max out
}

/**
 * @brief Test priority processing optimization
 */
TEST_F(QueueManagementOptimizationTest, testPriorityProcessingOptimization) {
    ASSERT_TRUE(monitor_->start());
    
    uint32_t high_priority_stream = 1;
    uint32_t normal_priority_stream = 2;
    
    // Set stream priorities
    monitor_->set_stream_priority(high_priority_stream, StreamPriority::HIGH);
    monitor_->set_stream_priority(normal_priority_stream, StreamPriority::NORMAL);
    
    std::vector<std::chrono::microseconds> high_priority_times;
    std::vector<std::chrono::microseconds> normal_priority_times;
    
    // Submit mixed priority frames rapidly
    for (size_t i = 0; i < 200; ++i) {
        auto frame_data = test_frames_[i % test_frames_.size()].to_byte_vector();
        
        if (i % 2 == 0) {
            // High priority frame
            auto start = std::chrono::high_resolution_clock::now();
            auto result = monitor_->process_eti_frame(high_priority_stream, frame_data);
            auto end = std::chrono::high_resolution_clock::now();
            
            auto processing_time = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
            high_priority_times.push_back(processing_time);
        } else {
            // Normal priority frame
            auto start = std::chrono::high_resolution_clock::now();
            auto result = monitor_->process_eti_frame(normal_priority_stream, frame_data);
            auto end = std::chrono::high_resolution_clock::now();
            
            auto processing_time = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
            normal_priority_times.push_back(processing_time);
        }
        
        // Brief pause every 50 frames
        if (i % 50 == 0) {
            std::this_thread::sleep_for(std::chrono::milliseconds(5));
        }
    }
    
    // Calculate average processing times
    auto avg_high_priority = std::accumulate(high_priority_times.begin(), 
                                           high_priority_times.end(),
                                           std::chrono::microseconds(0)) / high_priority_times.size();
    
    auto avg_normal_priority = std::accumulate(normal_priority_times.begin(),
                                             normal_priority_times.end(), 
                                             std::chrono::microseconds(0)) / normal_priority_times.size();
    
    // VALIDATE: High priority frames processed faster
    EXPECT_LT(avg_high_priority.count(), avg_normal_priority.count())
        << "High priority frames not processed faster than normal priority";
    
    // VALIDATE: Both stay within acceptable limits
    EXPECT_LT(avg_high_priority.count(), 10000); // <10ms for high priority
    EXPECT_LT(avg_normal_priority.count(), 50000); // <50ms for normal priority
    
    std::cout << "Priority Processing Results:" << std::endl;
    std::cout << "  High priority avg: " << avg_high_priority.count() << " μs" << std::endl;
    std::cout << "  Normal priority avg: " << avg_normal_priority.count() << " μs" << std::endl;
    std::cout << "  Priority advantage: " 
              << (avg_normal_priority.count() - avg_high_priority.count()) << " μs" << std::endl;
}

/**
 * @brief Test adaptive queue sizing optimization
 */
TEST_F(QueueManagementOptimizationTest, testAdaptiveQueueSizing) {
    // Configure monitor with adaptive queue sizing
    RealtimeMonitorConfig config;
    config.enable_adaptive_queue_sizing = true;
    config.initial_queue_size = 500;
    config.max_queue_size = 2000;
    config.queue_growth_factor = 1.5;
    config.queue_shrink_threshold = 0.3; // Shrink when utilization drops below 30%
    
    auto adaptive_monitor = std::make_unique<EtsiRealtimeMonitor>(config);
    ASSERT_TRUE(adaptive_monitor->initialize());
    ASSERT_TRUE(adaptive_monitor->start());
    
    uint32_t stream_id = 1;
    
    // Phase 1: Low load - should use smaller queue
    for (size_t i = 0; i < 50; ++i) {
        auto frame_data = test_frames_[i].to_byte_vector();
        adaptive_monitor->process_eti_frame(stream_id, frame_data);
        std::this_thread::sleep_for(std::chrono::milliseconds(50)); // Slow processing
    }
    
    auto stats_low_load = adaptive_monitor->get_queue_statistics();
    EXPECT_LE(stats_low_load.current_queue_size, 500); // Should use smaller queue
    
    // Phase 2: High load burst - should expand queue
    for (size_t i = 0; i < 500; ++i) {
        auto frame_data = test_frames_[i % test_frames_.size()].to_byte_vector();
        adaptive_monitor->process_eti_frame(stream_id, frame_data);
        // No delay - burst processing
    }
    
    auto stats_high_load = adaptive_monitor->get_queue_statistics();
    EXPECT_GT(stats_high_load.current_queue_size, 500); // Should expand queue
    EXPECT_LE(stats_high_load.current_queue_size, 2000); // But not exceed max
    
    // Phase 3: Return to low load - should shrink queue
    std::this_thread::sleep_for(std::chrono::milliseconds(1000)); // Allow processing to catch up
    
    for (size_t i = 0; i < 50; ++i) {
        auto frame_data = test_frames_[i].to_byte_vector();
        adaptive_monitor->process_eti_frame(stream_id, frame_data);
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }
    
    auto stats_return_low = adaptive_monitor->get_queue_statistics();
    EXPECT_LT(stats_return_low.current_queue_size, stats_high_load.current_queue_size); // Should shrink
    
    std::cout << "Adaptive Queue Sizing Results:" << std::endl;
    std::cout << "  Low load queue size: " << stats_low_load.current_queue_size << std::endl;
    std::cout << "  High load queue size: " << stats_high_load.current_queue_size << std::endl;
    std::cout << "  Return to low load: " << stats_return_low.current_queue_size << std::endl;
}

/**
 * @brief Test queue overflow strategies and recovery
 */
TEST_F(QueueManagementOptimizationTest, testQueueOverflowStrategiesAndRecovery) {
    // Test different overflow strategies
    std::vector<QueueOverflowStrategy> strategies = {
        QueueOverflowStrategy::DROP_OLDEST,
        QueueOverflowStrategy::DROP_NEWEST,
        QueueOverflowStrategy::PRIORITIZED_DROP,
        QueueOverflowStrategy::BACKPRESSURE
    };
    
    for (auto strategy : strategies) {
        // Configure monitor with specific overflow strategy
        RealtimeMonitorConfig config;
        config.max_queue_size = 100; // Small queue to trigger overflow
        config.queue_optimization = true;
        config.queue_overflow_strategy = strategy;
        
        auto test_monitor = std::make_unique<EtsiRealtimeMonitor>(config);
        ASSERT_TRUE(test_monitor->initialize());
        ASSERT_TRUE(test_monitor->start());
        
        uint32_t stream_id = 1;
        std::vector<ComplianceResult> results;
        
        // Submit enough frames to trigger overflow
        for (size_t i = 0; i < 200; ++i) {
            auto frame_data = test_frames_[i % test_frames_.size()].to_byte_vector();
            auto result = test_monitor->process_eti_frame(stream_id, frame_data);
            results.push_back(result);
        }
        
        // Allow processing to complete
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
        
        auto overflow_stats = test_monitor->get_queue_statistics();
        
        // VALIDATE: Strategy handled overflow appropriately
        switch (strategy) {
            case QueueOverflowStrategy::DROP_OLDEST:
            case QueueOverflowStrategy::DROP_NEWEST:
            case QueueOverflowStrategy::PRIORITIZED_DROP:
                // Should have dropped some frames but continued processing
                EXPECT_GT(overflow_stats.dropped_frames, 0);
                EXPECT_LT(overflow_stats.dropped_frames, 100); // Should not drop everything
                break;
                
            case QueueOverflowStrategy::BACKPRESSURE:
                // Should have applied backpressure without dropping
                EXPECT_EQ(overflow_stats.dropped_frames, 0);
                EXPECT_GT(overflow_stats.backpressure_events, 0);
                break;
        }
        
        // VALIDATE: Queue recovered after overflow
        EXPECT_LT(overflow_stats.current_queue_utilization_percent, 90.0);
        
        // VALIDATE: Processing continued despite overflow
        EXPECT_GT(overflow_stats.total_frames_processed, 100);
        
        test_monitor->stop();
        
        std::cout << "Overflow Strategy " << static_cast<int>(strategy) << " Results:" << std::endl;
        std::cout << "  Dropped frames: " << overflow_stats.dropped_frames << std::endl;
        std::cout << "  Backpressure events: " << overflow_stats.backpressure_events << std::endl;
        std::cout << "  Total processed: " << overflow_stats.total_frames_processed << std::endl;
    }
}

/**
 * @brief Test load balancing across processing threads
 */
TEST_F(QueueManagementOptimizationTest, testLoadBalancingOptimization) {
    // Configure monitor with multiple processing threads
    RealtimeMonitorConfig config;
    config.processing_thread_count = 4;
    config.enable_load_balancing = true;
    config.load_balancing_strategy = LoadBalancingStrategy::WORK_STEALING;
    config.queue_optimization = true;
    
    auto load_balanced_monitor = std::make_unique<EtsiRealtimeMonitor>(config);
    ASSERT_TRUE(load_balanced_monitor->initialize());
    ASSERT_TRUE(load_balanced_monitor->start());
    
    uint32_t stream_id = 1;
    std::atomic<size_t> total_processed(0);
    
    // Submit large batch of frames for load balancing
    auto start_time = std::chrono::high_resolution_clock::now();
    
    std::vector<std::future<void>> submission_futures;
    
    // Submit frames from multiple threads to test load balancing
    for (int thread_id = 0; thread_id < 4; ++thread_id) {
        auto future = std::async(std::launch::async, [&, thread_id]() {
            for (size_t i = 0; i < 250; ++i) {
                size_t frame_index = (thread_id * 250 + i) % test_frames_.size();
                auto frame_data = test_frames_[frame_index].to_byte_vector();
                
                auto result = load_balanced_monitor->process_eti_frame(stream_id + thread_id, frame_data);
                total_processed.fetch_add(1);
            }
        });
        
        submission_futures.push_back(std::move(future));
    }
    
    // Wait for all submissions to complete
    for (auto& future : submission_futures) {
        future.get();
    }
    
    auto end_time = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
    
    // VALIDATE: All frames processed
    EXPECT_EQ(total_processed.load(), 1000);
    
    // VALIDATE: Load balancing improved performance
    double fps = 1000.0 / (duration.count() / 1000.0);
    EXPECT_GT(fps, 2000.0); // Should achieve high throughput with load balancing
    
    // VALIDATE: Thread utilization was balanced
    auto thread_stats = load_balanced_monitor->get_thread_statistics();
    EXPECT_EQ(thread_stats.size(), 4); // Should have stats for all 4 threads
    
    // Check that work was distributed reasonably
    std::vector<size_t> thread_loads;
    for (const auto& stat : thread_stats) {
        thread_loads.push_back(stat.frames_processed);
    }
    
    auto min_load = *std::min_element(thread_loads.begin(), thread_loads.end());
    auto max_load = *std::max_element(thread_loads.begin(), thread_loads.end());
    
    // Load should be reasonably balanced (within 20% variance)
    double load_variance = static_cast<double>(max_load - min_load) / max_load;
    EXPECT_LT(load_variance, 0.3); // <30% variance acceptable
    
    std::cout << "Load Balancing Results:" << std::endl;
    std::cout << "  Total processing rate: " << fps << " FPS" << std::endl;
    std::cout << "  Load variance: " << (load_variance * 100.0) << "%" << std::endl;
    for (size_t i = 0; i < thread_loads.size(); ++i) {
        std::cout << "  Thread " << i << " processed: " << thread_loads[i] << " frames" << std::endl;
    }
}

/**
 * @brief Test queue monitoring and alerting integration
 */
TEST_F(QueueManagementOptimizationTest, testQueueMonitoringAndAlerting) {
    ASSERT_TRUE(monitor_->start());
    
    // Register alert callback to capture queue alerts
    std::vector<Alert> captured_alerts;
    alert_system_->register_alert_callback([&captured_alerts](const Alert& alert) {
        if (alert.category == AlertCategory::QUEUE_MANAGEMENT) {
            captured_alerts.push_back(alert);
        }
    });
    
    uint32_t stream_id = 1;
    
    // Phase 1: Normal operation - should not trigger alerts
    for (size_t i = 0; i < 100; ++i) {
        auto frame_data = test_frames_[i].to_byte_vector();
        monitor_->process_eti_frame(stream_id, frame_data);
        alert_system_->check_queue_status(monitor_->get_queue_statistics());
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    
    size_t normal_alerts = captured_alerts.size();
    EXPECT_EQ(normal_alerts, 0); // No alerts during normal operation
    
    // Phase 2: High load to trigger warning alerts
    for (size_t i = 0; i < 500; ++i) {
        auto frame_data = test_frames_[i % test_frames_.size()].to_byte_vector();
        monitor_->process_eti_frame(stream_id, frame_data);
        alert_system_->check_queue_status(monitor_->get_queue_statistics());
        // No delay - rapid submission
    }
    
    // Allow alert processing
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    
    // VALIDATE: Warning alerts generated for high queue utilization
    bool warning_alert_found = false;
    for (const auto& alert : captured_alerts) {
        if (alert.severity == AlertSeverity::WARNING && 
            alert.message.find("queue utilization") != std::string::npos) {
            warning_alert_found = true;
            break;
        }
    }
    EXPECT_TRUE(warning_alert_found) << "Queue warning alert not generated";
    
    // Phase 3: Allow queue to drain and verify recovery alerts
    std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    alert_system_->check_queue_status(monitor_->get_queue_statistics());
    
    // VALIDATE: Recovery alert generated
    bool recovery_alert_found = false;
    for (const auto& alert : captured_alerts) {
        if (alert.severity == AlertSeverity::INFO && 
            alert.message.find("queue utilization normalized") != std::string::npos) {
            recovery_alert_found = true;
            break;
        }
    }
    EXPECT_TRUE(recovery_alert_found) << "Queue recovery alert not generated";
    
    std::cout << "Queue Monitoring Results:" << std::endl;
    std::cout << "  Total alerts generated: " << captured_alerts.size() << std::endl;
    std::cout << "  Warning alerts: " << std::count_if(captured_alerts.begin(), captured_alerts.end(),
        [](const Alert& a) { return a.severity == AlertSeverity::WARNING; }) << std::endl;
    std::cout << "  Info alerts: " << std::count_if(captured_alerts.begin(), captured_alerts.end(),
        [](const Alert& a) { return a.severity == AlertSeverity::INFO; }) << std::endl;
}

/**
 * @brief Main test execution
 */
int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    
    std::cout << "\n🔄 QUEUE MANAGEMENT OPTIMIZATION VALIDATION TESTS" << std::endl;
    std::cout << "=================================================" << std::endl;
    std::cout << "Testing queue optimization preventing violations under high load..." << std::endl;
    
    auto start_time = std::chrono::high_resolution_clock::now();
    int result = RUN_ALL_TESTS();
    auto end_time = std::chrono::high_resolution_clock::now();
    
    auto total_time = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
    
    std::cout << "\n📊 QUEUE OPTIMIZATION TEST SUMMARY" << std::endl;
    std::cout << "===================================" << std::endl;
    std::cout << "Total execution time: " << total_time.count() << " ms" << std::endl;
    
    if (result == 0) {
        std::cout << "\n✅ ALL QUEUE TESTS PASSED - OPTIMIZATION VALIDATED" << std::endl;
        std::cout << "🚀 HIGH-LOAD PERFORMANCE MAINTAINED" << std::endl;
    } else {
        std::cout << "\n❌ QUEUE OPTIMIZATION TESTS FAILED - REQUIRES ATTENTION" << std::endl;
    }
    
    return result;
}