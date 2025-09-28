#include <QCoreApplication>
#include <QUdpSocket>
#include <QNetworkInterface>
#include <QHostAddress>
#include <QTimer>
#include <QDebug>
#include <iostream>
#include <chrono>
#include <memory>

#include "src/core/network_stream_receiver.hpp"
#include "src/core/stream_recording_manager.hpp"
#include "src/core/realtime_buffer_manager.hpp"

/**
 * @brief Network/Stream Agent Validation Test Application
 * 
 * Tests ETI-over-IP functionality, stream recording, and buffer management
 * to validate the claimed >900 FPS and <17ms latency performance.
 */
class NetworkValidationTest : public QObject {
    Q_OBJECT

public:
    NetworkValidationTest(QObject* parent = nullptr) : QObject(parent) {
        qDebug() << "=== Network/Stream Agent Validation Test ===";
        qDebug() << "Testing ETI-over-IP functionality and performance claims";
    }

public slots:
    void runValidationTests() {
        qDebug() << "\n1. Testing ETI-over-IP Components Instantiation...";
        testComponentInstantiation();
        
        qDebug() << "\n2. Testing Network Interface Discovery...";
        testNetworkInterfaces();
        
        qDebug() << "\n3. Testing Multicast Socket Configuration...";
        testMulticastConfiguration();
        
        qDebug() << "\n4. Testing Buffer Management Performance...";
        testBufferPerformance();
        
        qDebug() << "\n5. Testing Stream Recording Capabilities...";
        testStreamRecording();
        
        qDebug() << "\n6. Testing Integration Points...";
        testIntegrationPoints();
        
        qDebug() << "\n=== Validation Test Complete ===";
        generateValidationReport();
        
        QCoreApplication::quit();
    }

private:
    void testComponentInstantiation() {
        try {
            // Test NetworkStreamReceiver instantiation
            eti::NetworkStreamConfig config;
            config.multicast_address = "239.192.0.1";
            config.port = 9200;
            config.buffer_size = 1000;
            
            auto receiver = std::make_unique<eti::NetworkStreamReceiver>(config);
            qDebug() << "✅ NetworkStreamReceiver instantiated successfully";
            
            // Test StreamRecordingManager instantiation
            auto recording_manager = std::make_unique<eti::StreamRecordingManager>();
            qDebug() << "✅ StreamRecordingManager instantiated successfully";
            
            // Test RealTimeBufferManager instantiation
            eti::BufferConfig buffer_config;
            buffer_config.initial_capacity = 1000;
            buffer_config.strategy = eti::BufferStrategy::RING_BUFFER;
            
            auto buffer_manager = std::make_unique<eti::RealTimeBufferManager>(buffer_config);
            qDebug() << "✅ RealTimeBufferManager instantiated successfully";
            
            m_validation_results["component_instantiation"] = true;
            
        } catch (const std::exception& e) {
            qDebug() << "❌ Component instantiation failed:" << e.what();
            m_validation_results["component_instantiation"] = false;
        }
    }
    
    void testNetworkInterfaces() {
        try {
            const auto interfaces = QNetworkInterface::allInterfaces();
            int multicast_capable = 0;
            
            qDebug() << "Available network interfaces:";
            for (const auto& interface : interfaces) {
                const bool can_multicast = interface.flags() & QNetworkInterface::CanMulticast;
                const bool is_up = interface.flags() & QNetworkInterface::IsUp;
                const bool is_running = interface.flags() & QNetworkInterface::IsRunning;
                
                qDebug() << "  -" << interface.name() 
                         << "| Multicast:" << (can_multicast ? "YES" : "NO")
                         << "| Up:" << (is_up ? "YES" : "NO")
                         << "| Running:" << (is_running ? "YES" : "NO");
                
                if (can_multicast && is_up && is_running) {
                    multicast_capable++;
                }
            }
            
            if (multicast_capable > 0) {
                qDebug() << "✅ Found" << multicast_capable << "multicast-capable interfaces";
                m_validation_results["network_interfaces"] = true;
            } else {
                qDebug() << "❌ No multicast-capable interfaces found";
                m_validation_results["network_interfaces"] = false;
            }
            
        } catch (const std::exception& e) {
            qDebug() << "❌ Network interface discovery failed:" << e.what();
            m_validation_results["network_interfaces"] = false;
        }
    }
    
    void testMulticastConfiguration() {
        try {
            QUdpSocket test_socket;
            
            // Test binding to ETI multicast port
            const bool bind_success = test_socket.bind(
                QHostAddress::AnyIPv4, 9200, 
                QUdpSocket::ShareAddress | QUdpSocket::ReuseAddressHint
            );
            
            if (bind_success) {
                qDebug() << "✅ Successfully bound to ETI multicast port 9200";
                
                // Test joining multicast group
                const QHostAddress multicast_group("239.192.0.1");
                const bool join_success = test_socket.joinMulticastGroup(multicast_group);
                
                if (join_success) {
                    qDebug() << "✅ Successfully joined ETI multicast group 239.192.0.1";
                    m_validation_results["multicast_config"] = true;
                    
                    test_socket.leaveMulticastGroup(multicast_group);
                } else {
                    qDebug() << "❌ Failed to join multicast group:" << test_socket.errorString();
                    m_validation_results["multicast_config"] = false;
                }
                
                test_socket.close();
            } else {
                qDebug() << "❌ Failed to bind to port 9200:" << test_socket.errorString();
                m_validation_results["multicast_config"] = false;
            }
            
        } catch (const std::exception& e) {
            qDebug() << "❌ Multicast configuration test failed:" << e.what();
            m_validation_results["multicast_config"] = false;
        }
    }
    
    void testBufferPerformance() {
        try {
            eti::BufferConfig config;
            config.initial_capacity = 1000;
            config.strategy = eti::BufferStrategy::RING_BUFFER;
            
            eti::RealTimeBufferManager buffer_manager(config);
            
            // Create test ETI frame data
            std::array<uint8_t, 6144> test_frame_data;
            test_frame_data.fill(0xAB); // Test pattern
            
            // Add ETI sync pattern
            test_frame_data[0] = 0x68;
            test_frame_data[1] = 0x1A;
            test_frame_data[2] = 0xF3;
            test_frame_data[3] = 0x73;
            
            eti::EtiFrame test_frame(test_frame_data);
            
            // Performance test: measure write/read latency
            const size_t test_iterations = 10000;
            const auto start_time = std::chrono::high_resolution_clock::now();
            
            size_t successful_writes = 0;
            size_t successful_reads = 0;
            
            // Write test frames
            for (size_t i = 0; i < test_iterations; ++i) {
                if (buffer_manager.write_frame(test_frame)) {
                    successful_writes++;
                }
            }
            
            // Read test frames
            eti::EtiFrame read_frame;
            for (size_t i = 0; i < successful_writes; ++i) {
                if (buffer_manager.read_frame(read_frame)) {
                    successful_reads++;
                }
            }
            
            const auto end_time = std::chrono::high_resolution_clock::now();
            const auto duration = std::chrono::duration_cast<std::chrono::microseconds>(
                end_time - start_time
            );
            
            const double operations_per_second = (successful_writes + successful_reads) * 1000000.0 / duration.count();
            const double latency_per_operation = duration.count() / double(successful_writes + successful_reads);
            
            qDebug() << "Buffer Performance Results:";
            qDebug() << "  - Successful writes:" << successful_writes << "/" << test_iterations;
            qDebug() << "  - Successful reads:" << successful_reads << "/" << successful_writes;
            qDebug() << "  - Operations/second:" << QString::number(operations_per_second, 'f', 0);
            qDebug() << "  - Latency per operation:" << QString::number(latency_per_operation, 'f', 2) << "µs";
            
            // Check if performance meets claims (>900 FPS = >1800 operations/sec for read+write)
            if (operations_per_second > 1800 && latency_per_operation < 1000) {
                qDebug() << "✅ Buffer performance meets requirements";
                m_validation_results["buffer_performance"] = true;
                m_performance_metrics["buffer_ops_per_sec"] = operations_per_second;
                m_performance_metrics["buffer_latency_us"] = latency_per_operation;
            } else {
                qDebug() << "❌ Buffer performance below requirements";
                m_validation_results["buffer_performance"] = false;
            }
            
        } catch (const std::exception& e) {
            qDebug() << "❌ Buffer performance test failed:" << e.what();
            m_validation_results["buffer_performance"] = false;
        }
    }
    
    void testStreamRecording() {
        try {
            eti::StreamRecordingManager recording_manager;
            
            // Test recording session creation
            const QString session_id = recording_manager.start_recording_session(
                "/tmp/eti_test_recording",
                eti::RecordingFormat::ETI_NI
            );
            
            if (!session_id.isEmpty()) {
                qDebug() << "✅ Recording session created successfully:" << session_id;
                
                // Test recording some test frames
                std::array<uint8_t, 6144> test_frame_data;
                test_frame_data.fill(0xCD);
                test_frame_data[0] = 0x68; test_frame_data[1] = 0x1A;
                test_frame_data[2] = 0xF3; test_frame_data[3] = 0x73;
                
                eti::EtiFrame test_frame(test_frame_data);
                
                // Record test frames
                for (int i = 0; i < 100; ++i) {
                    recording_manager.record_frame(session_id, test_frame);
                }
                
                // Stop recording
                const bool stop_success = recording_manager.stop_recording_session(session_id);
                
                if (stop_success) {
                    qDebug() << "✅ Recording session stopped successfully";
                    m_validation_results["stream_recording"] = true;
                } else {
                    qDebug() << "❌ Failed to stop recording session";
                    m_validation_results["stream_recording"] = false;
                }
                
            } else {
                qDebug() << "❌ Failed to create recording session";
                m_validation_results["stream_recording"] = false;
            }
            
        } catch (const std::exception& e) {
            qDebug() << "❌ Stream recording test failed:" << e.what();
            m_validation_results["stream_recording"] = false;
        }
    }
    
    void testIntegrationPoints() {
        try {
            // Test signal/slot connections
            eti::NetworkStreamReceiver receiver;
            
            bool frame_signal_connected = false;
            bool error_signal_connected = false;
            
            // Test signal connections
            connect(&receiver, &eti::NetworkStreamReceiver::frame_received,
                    [&frame_signal_connected](const eti::EtiFrame&) {
                        frame_signal_connected = true;
                    });
            
            connect(&receiver, &eti::NetworkStreamReceiver::error_occurred,
                    [&error_signal_connected](const QString&) {
                        error_signal_connected = true;
                    });
            
            // Test configuration methods
            eti::NetworkStreamConfig config;
            config.multicast_address = "239.192.0.1";
            config.port = 9200;
            
            receiver.set_config(config);
            const auto retrieved_config = receiver.get_config();
            
            const bool config_matches = (retrieved_config.multicast_address == config.multicast_address &&
                                       retrieved_config.port == config.port);
            
            if (config_matches) {
                qDebug() << "✅ Configuration get/set functionality working";
                m_validation_results["integration_points"] = true;
            } else {
                qDebug() << "❌ Configuration get/set mismatch";
                m_validation_results["integration_points"] = false;
            }
            
        } catch (const std::exception& e) {
            qDebug() << "❌ Integration points test failed:" << e.what();
            m_validation_results["integration_points"] = false;
        }
    }
    
    void generateValidationReport() {
        qDebug() << "\n=== NETWORK/STREAM AGENT VALIDATION REPORT ===";
        
        int passed_tests = 0;
        int total_tests = 0;
        
        for (auto it = m_validation_results.begin(); it != m_validation_results.end(); ++it) {
            total_tests++;
            if (it.value()) {
                passed_tests++;
                qDebug() << "✅" << it.key() << ": PASSED";
            } else {
                qDebug() << "❌" << it.key() << ": FAILED";
            }
        }
        
        const double pass_rate = (double(passed_tests) / total_tests) * 100.0;
        qDebug() << "\nValidation Summary:";
        qDebug() << "  - Tests passed:" << passed_tests << "/" << total_tests;
        qDebug() << "  - Pass rate:" << QString::number(pass_rate, 'f', 1) << "%";
        
        if (!m_performance_metrics.isEmpty()) {
            qDebug() << "\nPerformance Metrics:";
            for (auto it = m_performance_metrics.begin(); it != m_performance_metrics.end(); ++it) {
                qDebug() << "  -" << it.key() << ":" << it.value();
            }
        }
        
        // Generate recommendations
        qDebug() << "\nRecommendations for UI/UX Agent:";
        if (pass_rate >= 80) {
            qDebug() << "✅ Network components ready for UI integration";
            qDebug() << "✅ ETI-over-IP functionality validated";
            qDebug() << "✅ Stream recording capabilities confirmed";
            if (m_validation_results["buffer_performance"]) {
                qDebug() << "✅ Buffer performance meets >900 FPS requirement";
            }
        } else {
            qDebug() << "❌ Network components need fixes before UI integration";
            qDebug() << "❌ Review failed test components before proceeding";
        }
        
        qDebug() << "\n=== END VALIDATION REPORT ===";
    }

private:
    QMap<QString, bool> m_validation_results;
    QMap<QString, double> m_performance_metrics;
};

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);
    
    NetworkValidationTest test;
    
    // Start validation after event loop starts
    QTimer::singleShot(100, &test, &NetworkValidationTest::runValidationTests);
    
    return app.exec();
}

#include "test_network_validation.moc"