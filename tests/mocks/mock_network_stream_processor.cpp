#include "mock_network_stream_processor.h"
#include "etsi_compliant_frames.h"
#include <chrono>
#include <random>

using ::testing::_;
using ::testing::Return;
using ::testing::ReturnRef;
using ::testing::Invoke;
using ::testing::DoAll;
using ::testing::SetArgReferee;

namespace StreamMocks {

// NetworkStreamMockFactory implementations
std::unique_ptr<MockRealTimeEtiProcessor> NetworkStreamMockFactory::create_high_performance_processor() {
    auto mock = std::make_unique<MockRealTimeEtiProcessor>();
    
    // Configure for >900 frames/second capability
    ON_CALL(*mock, start_processing())
        .WillByDefault(Return(true));
    
    ON_CALL(*mock, is_processing())
        .WillByDefault(Return(true));
    
    ON_CALL(*mock, process_frame_realtime(_))
        .WillByDefault(Return(true));
    
    ON_CALL(*mock, process_frame_batch(_))
        .WillByDefault(Return(true));
    
    // Performance metrics exceeding target
    ON_CALL(*mock, get_processing_rate())
        .WillByDefault(Return(950.0)); // >900 fps
    
    ON_CALL(*mock, meets_performance_target(_))
        .WillByDefault([](double target_fps) {
            return target_fps <= 950.0;
        });
    
    ON_CALL(*mock, get_average_processing_time())
        .WillByDefault(Return(std::chrono::microseconds(1000))); // 1ms avg
    
    ON_CALL(*mock, get_max_processing_time())
        .WillByDefault(Return(std::chrono::microseconds(2500))); // 2.5ms max
    
    // Queue management for high throughput
    ON_CALL(*mock, get_queue_length())
        .WillByDefault(Return(50)); // Manageable queue depth
    
    ON_CALL(*mock, enqueue_frame(_))
        .WillByDefault(Return(true));
    
    ON_CALL(*mock, dequeue_frame(_))
        .WillByDefault(DoAll(
            Invoke([](EtiFrame& frame) {
                // Simulate dequeuing a valid frame
                frame = EtsiTestData::GERMAN_PUBLIC_RADIO_ENSEMBLE;
            }),
            Return(true)
        ));
    
    // Threading configuration
    ON_CALL(*mock, get_active_thread_count())
        .WillByDefault(Return(4)); // Multi-threaded processing
    
    // Memory efficiency
    ON_CALL(*mock, get_memory_usage())
        .WillByDefault(Return(64 * 1024 * 1024)); // 64MB reasonable usage
    
    ON_CALL(*mock, is_memory_limit_exceeded())
        .WillByDefault(Return(false));
    
    // Detailed performance metrics
    ON_CALL(*mock, get_detailed_metrics())
        .WillByDefault([]() {
            PerformanceMetrics metrics;
            metrics.current_fps = 920.0;
            metrics.average_fps = 915.0;
            metrics.peak_fps = 980.0;
            metrics.min_processing_time = std::chrono::microseconds(800);
            metrics.avg_processing_time = std::chrono::microseconds(1000);
            metrics.max_processing_time = std::chrono::microseconds(2500);
            metrics.cpu_usage = 65.0; // 65% CPU
            metrics.memory_usage = 64 * 1024 * 1024;
            metrics.network_bandwidth = 2.048; // 2.048 Mbps
            metrics.meets_target = true;
            return metrics;
        });
    
    return mock;
}

std::unique_ptr<MockNetworkStreamReceiver> NetworkStreamMockFactory::create_realistic_network_receiver() {
    auto mock = std::make_unique<MockNetworkStreamReceiver>();
    
    // Connection management
    ON_CALL(*mock, connect(_, _))
        .WillByDefault(Return(true));
    
    ON_CALL(*mock, connect_with_config(_))
        .WillByDefault(Return(true));
    
    ON_CALL(*mock, is_connected())
        .WillByDefault(Return(true));
    
    ON_CALL(*mock, is_receiving())
        .WillByDefault(Return(true));
    
    // Realistic frame reception (250 fps standard)
    static size_t frame_counter = 0;
    ON_CALL(*mock, receive_eti_frame())
        .WillByDefault(Invoke([]() {
            // Simulate receiving ETSI-compliant frames
            auto frame = EtsiTestData::GERMAN_PUBLIC_RADIO_ENSEMBLE;
            frame.set_frame_count(static_cast<uint8_t>((frame_counter++) % 250));
            return std::vector<uint8_t>(frame.data.begin(), frame.data.end());
        }));
    
    ON_CALL(*mock, receive_packet_batch(_))
        .WillByDefault([](size_t max_packets) {
            std::vector<NetworkPacket> packets;
            packets.reserve(max_packets);
            
            for (size_t i = 0; i < max_packets; ++i) {
                NetworkPacket packet;
                auto frame = EtsiTestData::GERMAN_PUBLIC_RADIO_ENSEMBLE;
                packet.data = std::vector<uint8_t>(frame.data.begin(), frame.data.end());
                packet.timestamp = std::chrono::steady_clock::now();
                packet.sequence_number = frame_counter + i;
                packet.is_valid = true;
                packets.push_back(packet);
            }
            
            return packets;
        });
    
    ON_CALL(*mock, has_pending_data())
        .WillByDefault(Return(true));
    
    ON_CALL(*mock, get_available_frame_count())
        .WillByDefault(Return(10)); // Typical buffer depth
    
    // Buffer management
    ON_CALL(*mock, get_buffer_status())
        .WillByDefault([]() {
            BufferStatus status;
            status.capacity = 1000;
            status.used = 250;
            status.free = 750;
            status.utilization = 25.0;
            status.overflow = false;
            status.underflow = false;
            status.latency = std::chrono::milliseconds(96); // 4 frames @ 24ms
            return status;
        });
    
    ON_CALL(*mock, is_buffer_overflow())
        .WillByDefault(Return(false));
    
    ON_CALL(*mock, is_buffer_underflow())
        .WillByDefault(Return(false));
    
    // Performance monitoring
    ON_CALL(*mock, get_stream_statistics())
        .WillByDefault([]() {
            StreamStatistics stats;
            stats.frames_received = 25000; // 100 seconds @ 250fps
            stats.frames_processed = 24995;
            stats.frames_dropped = 5;
            stats.errors_detected = 2;
            stats.average_frame_rate = 249.8;
            stats.average_latency = std::chrono::microseconds(96000);
            stats.start_time = std::chrono::steady_clock::now() - std::chrono::seconds(100);
            stats.last_update = std::chrono::steady_clock::now();
            return stats;
        });
    
    ON_CALL(*mock, get_performance_metrics())
        .WillByDefault([]() {
            PerformanceMetrics metrics;
            metrics.current_fps = 249.8;
            metrics.average_fps = 249.9;
            metrics.peak_fps = 250.0;
            metrics.avg_processing_time = std::chrono::microseconds(500);
            metrics.cpu_usage = 25.0;
            metrics.memory_usage = 32 * 1024 * 1024; // 32MB
            metrics.network_bandwidth = 2.048;
            metrics.meets_target = true;
            return metrics;
        });
    
    ON_CALL(*mock, get_frame_rate())
        .WillByDefault(Return(249.8));
    
    ON_CALL(*mock, get_frames_received())
        .WillByDefault(Return(25000));
    
    ON_CALL(*mock, get_frames_dropped())
        .WillByDefault(Return(5));
    
    // Error handling
    ON_CALL(*mock, get_error_messages())
        .WillByDefault(Return(std::vector<std::string>{
            "Minor sync glitch at frame 12543",
            "Network jitter detected at 15:32:45"
        }));
    
    ON_CALL(*mock, has_errors())
        .WillByDefault(Return(false)); // No critical errors
    
    ON_CALL(*mock, get_last_error())
        .WillByDefault(Return(std::string{}));
    
    return mock;
}

std::unique_ptr<MockPerformanceMonitor> NetworkStreamMockFactory::create_stress_test_monitor() {
    auto mock = std::make_unique<MockPerformanceMonitor>();
    
    // Monitoring control
    ON_CALL(*mock, start_monitoring())
        .WillByDefault(Return());
    
    ON_CALL(*mock, is_monitoring())
        .WillByDefault(Return(true));
    
    // Stress test metrics - showing system under high load
    static double current_fps = 850.0; // Just below target to show stress
    static std::mt19937 rng(12345);
    static std::uniform_real_distribution<double> fps_jitter(840.0, 860.0);
    
    ON_CALL(*mock, get_current_fps())
        .WillByDefault([&]() {
            current_fps = fps_jitter(rng); // Simulate load fluctuation
            return current_fps;
        });
    
    ON_CALL(*mock, get_average_fps())
        .WillByDefault(Return(847.5)); // Slightly below target
    
    ON_CALL(*mock, get_peak_fps())
        .WillByDefault(Return(895.0)); // Peak approaching target
    
    ON_CALL(*mock, is_meeting_target(_))
        .WillByDefault([](double target_fps) {
            return target_fps <= 850.0; // Sometimes fails 900fps target
        });
    
    // Elevated latency under stress
    ON_CALL(*mock, get_current_latency())
        .WillByDefault(Return(std::chrono::microseconds(1500)));
    
    ON_CALL(*mock, get_average_latency())
        .WillByDefault(Return(std::chrono::microseconds(1200)));
    
    ON_CALL(*mock, get_max_latency())
        .WillByDefault(Return(std::chrono::microseconds(3500)));
    
    // High resource usage under stress
    ON_CALL(*mock, get_cpu_usage())
        .WillByDefault(Return(92.0)); // High CPU load
    
    ON_CALL(*mock, get_memory_usage())
        .WillByDefault(Return(256 * 1024 * 1024)); // 256MB under stress
    
    ON_CALL(*mock, get_network_bandwidth())
        .WillByDefault(Return(2.1)); // Slightly over nominal
    
    // Comprehensive stress metrics
    ON_CALL(*mock, get_comprehensive_metrics())
        .WillByDefault([]() {
            PerformanceMetrics metrics;
            metrics.current_fps = 845.0;
            metrics.average_fps = 847.5;
            metrics.peak_fps = 895.0;
            metrics.min_processing_time = std::chrono::microseconds(900);
            metrics.avg_processing_time = std::chrono::microseconds(1200);
            metrics.max_processing_time = std::chrono::microseconds(3500);
            metrics.cpu_usage = 92.0;
            metrics.memory_usage = 256 * 1024 * 1024;
            metrics.network_bandwidth = 2.1;
            metrics.meets_target = false; // Failing under stress
            return metrics;
        });
    
    ON_CALL(*mock, generate_performance_report())
        .WillByDefault(Return(std::vector<std::string>{
            "STRESS TEST PERFORMANCE REPORT",
            "================================",
            "Current FPS: 845.0 (Target: 900.0) - BELOW TARGET",
            "Average FPS: 847.5",
            "Peak FPS: 895.0",
            "CPU Usage: 92.0% - HIGH",
            "Memory Usage: 256MB - ELEVATED",
            "Network Bandwidth: 2.1 Mbps",
            "Average Latency: 1.2ms",
            "Max Latency: 3.5ms - HIGH",
            "",
            "RECOMMENDATION: System under stress, consider optimization",
            "CRITICAL: Performance target not consistently met"
        }));
    
    return mock;
}

std::tuple<
    std::unique_ptr<MockNetworkStreamReceiver>,
    std::unique_ptr<MockRealTimeEtiProcessor>,
    std::unique_ptr<MockStreamBufferManager>,
    std::unique_ptr<MockPerformanceMonitor>
> NetworkStreamMockFactory::create_complete_stream_pipeline() {
    
    auto receiver = create_realistic_network_receiver();
    auto processor = create_high_performance_processor();
    auto buffer_manager = std::make_unique<MockStreamBufferManager>();
    auto monitor = std::make_unique<MockPerformanceMonitor>();
    
    // Configure buffer manager for pipeline
    ON_CALL(*buffer_manager, add_frame(_))
        .WillByDefault(Return(true));
    
    ON_CALL(*buffer_manager, get_frame(_))
        .WillByDefault(DoAll(
            Invoke([](EtiFrame& frame) {
                frame = EtsiTestData::GERMAN_PUBLIC_RADIO_ENSEMBLE;
            }),
            Return(true)
        ));
    
    ON_CALL(*buffer_manager, get_buffer_size())
        .WillByDefault(Return(1000));
    
    ON_CALL(*buffer_manager, get_used_capacity())
        .WillByDefault(Return(250));
    
    ON_CALL(*buffer_manager, get_utilization_percentage())
        .WillByDefault(Return(25.0));
    
    ON_CALL(*buffer_manager, get_current_latency())
        .WillByDefault(Return(std::chrono::milliseconds(96)));
    
    ON_CALL(*buffer_manager, is_latency_acceptable())
        .WillByDefault(Return(true));
    
    ON_CALL(*buffer_manager, get_buffer_statistics())
        .WillByDefault([]() {
            BufferStatus status;
            status.capacity = 1000;
            status.used = 250;
            status.free = 750;
            status.utilization = 25.0;
            status.overflow = false;
            status.underflow = false;
            status.latency = std::chrono::milliseconds(96);
            return status;
        });
    
    // Configure monitor for pipeline
    ON_CALL(*monitor, is_monitoring())
        .WillByDefault(Return(true));
    
    ON_CALL(*monitor, get_current_fps())
        .WillByDefault(Return(925.0)); // Good pipeline performance
    
    ON_CALL(*monitor, get_average_fps())
        .WillByDefault(Return(920.0));
    
    ON_CALL(*monitor, is_meeting_target(_))
        .WillByDefault([](double target) { return target <= 925.0; });
    
    ON_CALL(*monitor, get_comprehensive_metrics())
        .WillByDefault([]() {
            PerformanceMetrics metrics;
            metrics.current_fps = 925.0;
            metrics.average_fps = 920.0;
            metrics.peak_fps = 950.0;
            metrics.avg_processing_time = std::chrono::microseconds(900);
            metrics.cpu_usage = 70.0;
            metrics.memory_usage = 96 * 1024 * 1024;
            metrics.network_bandwidth = 2.048;
            metrics.meets_target = true;
            return metrics;
        });
    
    return std::make_tuple(
        std::move(receiver),
        std::move(processor), 
        std::move(buffer_manager),
        std::move(monitor)
    );
}

std::tuple<
    std::unique_ptr<MockFigProcessor>,
    std::unique_ptr<MockAudioStreamDecoder>,
    std::unique_ptr<MockEtiStreamValidator>
> NetworkStreamMockFactory::create_gui_testing_mocks() {
    
    auto fig_processor = std::make_unique<MockFigProcessor>();
    auto audio_decoder = std::make_unique<MockAudioStreamDecoder>();
    auto validator = std::make_unique<MockEtiStreamValidator>();
    
    // Configure FIG processor for GUI display
    ON_CALL(*fig_processor, process_fig_block(_))
        .WillByDefault(Return(true));
    
    ON_CALL(*fig_processor, extract_ensemble_info(_, _))
        .WillByDefault(DoAll(
            SetArgReferee<0>(0xD001), // German ensemble ID
            SetArgReferee<1>(std::string("German Public Radio")),
            Return(true)
        ));
    
    ON_CALL(*fig_processor, get_ensemble_id())
        .WillByDefault(Return(0xD001));
    
    ON_CALL(*fig_processor, get_ensemble_label())
        .WillByDefault(Return("German Public Radio"));
    
    ON_CALL(*fig_processor, get_service_ids())
        .WillByDefault(Return(std::vector<uint16_t>{0x1001, 0x1002, 0x1003}));
    
    ON_CALL(*fig_processor, get_service_labels())
        .WillByDefault(Return(std::map<uint16_t, std::string>{
            {0x1001, "Deutschlandfunk"},
            {0x1002, "WDR Radio"},
            {0x1003, "Bayern 1"}
        }));
    
    ON_CALL(*fig_processor, is_configuration_complete())
        .WillByDefault(Return(true));
    
    // Configure audio decoder for GUI
    ON_CALL(*audio_decoder, decode_audio_stream(_))
        .WillByDefault(Return(true));
    
    ON_CALL(*audio_decoder, detect_audio_format(_))
        .WillByDefault(Return("DAB"));
    
    ON_CALL(*audio_decoder, get_sample_rate())
        .WillByDefault(Return(48000));
    
    ON_CALL(*audio_decoder, get_bit_rate())
        .WillByDefault(Return(128));
    
    ON_CALL(*audio_decoder, get_channel_count())
        .WillByDefault(Return(2));
    
    ON_CALL(*audio_decoder, has_audio_output())
        .WillByDefault(Return(true));
    
    ON_CALL(*audio_decoder, get_signal_quality())
        .WillByDefault(Return(0.95)); // 95% quality
    
    // Configure validator for GUI
    ON_CALL(*validator, validate_frame(_))
        .WillByDefault(Return(true));
    
    ON_CALL(*validator, is_etsi_compliant(_))
        .WillByDefault(Return(true));
    
    ON_CALL(*validator, calculate_compliance_score(_))
        .WillByDefault(Return(0.98)); // 98% compliance
    
    ON_CALL(*validator, get_compliance_violations(_))
        .WillByDefault(Return(std::vector<std::string>{})); // No violations
    
    return std::make_tuple(
        std::move(fig_processor),
        std::move(audio_decoder),
        std::move(validator)
    );
}

std::unique_ptr<MockEtiStreamValidator> NetworkStreamMockFactory::create_build_validation_mock() {
    auto mock = std::make_unique<MockEtiStreamValidator>();
    
    // Configure for build system integration testing
    ON_CALL(*mock, validate_frame(_))
        .WillByDefault(Return(true));
    
    ON_CALL(*mock, validate_frame_sequence(_))
        .WillByDefault(Return(true));
    
    ON_CALL(*mock, is_etsi_compliant(_))
        .WillByDefault(Return(true));
    
    ON_CALL(*mock, calculate_compliance_score(_))
        .WillByDefault(Return(1.0)); // Perfect compliance for build tests
    
    ON_CALL(*mock, analyze_stream(_))
        .WillByDefault([]() {
            StreamStatistics stats;
            stats.frames_received = 1000;
            stats.frames_processed = 1000;
            stats.frames_dropped = 0;
            stats.errors_detected = 0;
            stats.average_frame_rate = 250.0;
            return stats;
        });
    
    ON_CALL(*mock, generate_validation_report(_))
        .WillByDefault(Return(std::vector<std::string>{
            "BUILD VALIDATION REPORT",
            "======================",
            "Frames Validated: 1000",
            "ETSI Compliance: 100%",
            "Errors Detected: 0",
            "Result: PASS - All tests successful"
        }));
    
    return mock;
}

std::unique_ptr<MockEtiStreamValidator> NetworkStreamMockFactory::create_compliance_testing_mock() {
    auto mock = std::make_unique<MockEtiStreamValidator>();
    
    // Configure for comprehensive ETSI compliance testing
    ON_CALL(*mock, validate_sync_pattern(_))
        .WillByDefault(Return(true));
    
    ON_CALL(*mock, validate_frame_structure(_))
        .WillByDefault(Return(true));
    
    ON_CALL(*mock, validate_crc(_))
        .WillByDefault(Return(true));
    
    ON_CALL(*mock, validate_frame_timing(_))
        .WillByDefault(Return(true));
    
    ON_CALL(*mock, validate_tist_sequence(_))
        .WillByDefault(Return(true));
    
    ON_CALL(*mock, is_etsi_compliant(_))
        .WillByDefault(Return(true));
    
    ON_CALL(*mock, calculate_compliance_score(_))
        .WillByDefault(Return(0.995)); // Near-perfect compliance
    
    ON_CALL(*mock, get_compliance_violations(_))
        .WillByDefault(Return(std::vector<std::string>{
            "Minor: FIC padding not optimal (non-critical)"
        }));
    
    ON_CALL(*mock, calculate_timing_drift(_))
        .WillByDefault(Return(std::chrono::microseconds(50))); // 50µs drift
    
    ON_CALL(*mock, generate_validation_report(_))
        .WillByDefault(Return(std::vector<std::string>{
            "ETSI COMPLIANCE VALIDATION REPORT",
            "=================================",
            "Standard: ETSI EN 300 799 v1.2.1",
            "Test Date: " + std::to_string(std::chrono::system_clock::to_time_t(std::chrono::system_clock::now())),
            "",
            "COMPLIANCE SUMMARY:",
            "Overall Score: 99.5%",
            "Sync Pattern: PASS",
            "Frame Structure: PASS", 
            "CRC Validation: PASS",
            "Timing Validation: PASS",
            "TIST Sequence: PASS",
            "",
            "MINOR ISSUES:",
            "- FIC padding not optimal (non-critical)",
            "",
            "RESULT: COMPLIANT",
            "The tested ETI stream meets ETSI EN 300 799 requirements",
            "suitable for broadcast use."
        }));
    
    return mock;
}

} // namespace StreamMocks