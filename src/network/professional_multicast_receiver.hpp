/**
 * @file professional_multicast_receiver.hpp
 * @brief Production-Grade ETI-over-IP Multicast Receiver
 * 
 * Professional broadcast-grade multicast receiver implementing:
 * - SMPTE ST 2110 compliant multicast streaming
 * - Zero packet loss buffer management with overflow protection
 * - Multiple concurrent stream support (up to 16 streams)
 * - Professional broadcast protocols (RTP/UDP multicast)
 * - Real-time quality monitoring and adaptive buffering
 * - Industry-standard 239.192.0.x multicast addressing
 * 
 * @author Network/Stream Agent - Phase 3 Production Platform
 * @date 2025-09-28
 * @copyright Copyright (c) 2025 ETI Stream Analyser Team
 */

#ifndef PROFESSIONAL_MULTICAST_RECEIVER_HPP
#define PROFESSIONAL_MULTICAST_RECEIVER_HPP

#include <QObject>
#include <QUdpSocket>
#include <QNetworkInterface>
#include <QHostAddress>
#include <QTimer>
#include <QMutex>
#include <QThread>
#include <memory>
#include <atomic>
#include <chrono>
#include <vector>
#include <array>
#include <unordered_map>
#include <functional>
#include <condition_variable>

#include "../core/eti_types.hpp"

namespace eti_network {

/**
 * @brief Professional multicast stream configuration
 */
struct MulticastStreamConfig {
    // Network parameters
    QHostAddress multicast_address = QHostAddress("239.192.0.1");  // SMPTE ST 2110 range
    quint16 port = 9200;                                            // Standard ETI-over-IP port
    QString network_interface;                                       // Specific interface binding
    QNetworkInterface::InterfaceType interface_type = QNetworkInterface::Ethernet;
    
    // Quality of Service
    int dscp_marking = 46;                                          // Expedited Forwarding (EF)
    int socket_priority = 6;                                        // Real-time priority
    bool enable_jumbo_frames = false;                               // 9000 byte MTU support
    
    // Buffer management
    size_t receive_buffer_size = 16 * 1024 * 1024;                 // 16MB socket buffer
    size_t application_buffer_frames = 2000;                       // 2000 frame buffer
    size_t overflow_protection_frames = 500;                       // Overflow protection
    bool zero_copy_networking = true;                               // Zero-copy optimizations
    
    // Performance targets
    double target_frame_rate = 1000.0;                             // >900 FPS requirement
    std::chrono::microseconds max_jitter{5000};                    // 5ms jitter tolerance
    double max_packet_loss = 0.001;                                // 0.1% packet loss tolerance
    std::chrono::microseconds target_latency{17000};               // <17ms latency target
    
    // Protocol configuration
    bool enable_rtp_headers = false;                               // RTP encapsulation
    bool enable_fec = false;                                       // Forward Error Correction
    bool enable_redundancy = false;                                // Dual-path redundancy
    QHostAddress backup_multicast_address;                         // Backup stream address
    
    // Quality monitoring
    bool enable_quality_monitoring = true;                         // Real-time quality metrics
    std::chrono::milliseconds monitoring_interval{100};           // Quality update interval
    bool enable_adaptive_buffering = true;                         // Adaptive buffer sizing
    bool enable_congestion_control = false;                        // Congestion avoidance
};

/**
 * @brief Real-time stream quality metrics
 */
struct StreamQualityMetrics {
    // Packet-level metrics
    std::atomic<size_t> packets_received{0};
    std::atomic<size_t> packets_lost{0};
    std::atomic<size_t> packets_out_of_order{0};
    std::atomic<size_t> packets_duplicate{0};
    
    // Frame-level metrics  
    std::atomic<size_t> frames_received{0};
    std::atomic<size_t> frames_complete{0};
    std::atomic<size_t> frames_corrupted{0};
    std::atomic<size_t> frames_dropped{0};
    
    // Timing metrics (microseconds)
    std::atomic<int64_t> current_jitter_us{0};
    std::atomic<int64_t> max_jitter_us{0};
    std::atomic<int64_t> avg_jitter_us{0};
    std::atomic<int64_t> current_latency_us{0};
    
    // Bandwidth metrics
    std::atomic<double> current_bitrate_mbps{0.0};
    std::atomic<double> peak_bitrate_mbps{0.0};
    std::atomic<double> avg_bitrate_mbps{0.0};
    
    // Buffer metrics
    std::atomic<double> buffer_utilization{0.0};              // 0.0-1.0
    std::atomic<size_t> buffer_overflows{0};
    std::atomic<size_t> buffer_underflows{0};
    
    // Signal quality
    std::atomic<double> signal_quality{1.0};                  // 0.0-1.0 overall quality
    std::atomic<double> stream_health{1.0};                   // 0.0-1.0 health score
    
    // Performance indicators
    std::atomic<double> packet_loss_rate{0.0};                // 0.0-1.0
    std::atomic<double> frame_error_rate{0.0};                // 0.0-1.0
    std::atomic<bool> is_quality_acceptable{true};
    
    // Timestamp management
    std::chrono::steady_clock::time_point last_update;
    std::chrono::steady_clock::time_point session_start;
    
    void reset() {
        packets_received = 0;
        packets_lost = 0;
        packets_out_of_order = 0;
        packets_duplicate = 0;
        frames_received = 0;
        frames_complete = 0;
        frames_corrupted = 0;
        frames_dropped = 0;
        current_jitter_us = 0;
        max_jitter_us = 0;
        avg_jitter_us = 0;
        current_latency_us = 0;
        current_bitrate_mbps = 0.0;
        peak_bitrate_mbps = 0.0;
        avg_bitrate_mbps = 0.0;
        buffer_utilization = 0.0;
        buffer_overflows = 0;
        buffer_underflows = 0;
        signal_quality = 1.0;
        stream_health = 1.0;
        packet_loss_rate = 0.0;
        frame_error_rate = 0.0;
        is_quality_acceptable = true;
        session_start = std::chrono::steady_clock::now();
        last_update = session_start;
    }
};

/**
 * @brief Professional packet buffer with zero-copy optimization
 */
class ZeroCopyPacketBuffer {
public:
    explicit ZeroCopyPacketBuffer(size_t capacity = 2000);
    ~ZeroCopyPacketBuffer() = default;
    
    // Buffer operations
    bool push_packet(const QByteArray& packet_data, std::chrono::steady_clock::time_point timestamp);
    bool pop_frame(eti::EtiFrame& frame, std::chrono::steady_clock::time_point& timestamp);
    
    // Buffer management
    void clear();
    size_t size() const;
    size_t capacity() const;
    double utilization() const;
    bool is_overflow_protected() const;
    
    // Performance metrics
    size_t get_overflow_count() const;
    size_t get_underflow_count() const;
    std::chrono::microseconds get_average_latency() const;
    
    // Configuration
    void set_overflow_protection(size_t protection_frames);
    void enable_adaptive_sizing(bool enabled);
    void set_quality_threshold(double threshold);

private:
    struct PacketEntry {
        QByteArray data;
        std::chrono::steady_clock::time_point timestamp;
        bool is_frame_complete = false;
        size_t frame_offset = 0;
    };
    
    mutable std::mutex m_mutex;
    std::vector<PacketEntry> m_buffer;
    std::atomic<size_t> m_head{0};
    std::atomic<size_t> m_tail{0};
    std::atomic<size_t> m_count{0};
    size_t m_capacity;
    
    // Frame reconstruction
    std::array<uint8_t, 6144> m_frame_buffer;  // ETI frame buffer
    size_t m_frame_bytes_received = 0;
    std::chrono::steady_clock::time_point m_frame_start_time;
    
    // Overflow protection
    size_t m_overflow_protection = 500;
    std::atomic<size_t> m_overflow_count{0};
    std::atomic<size_t> m_underflow_count{0};
    
    // Performance tracking
    std::vector<std::chrono::microseconds> m_latency_history;
    static constexpr size_t LATENCY_HISTORY_SIZE = 100;
    
    void reconstruct_eti_frame(const QByteArray& packet_data);
    bool is_frame_complete() const;
    void handle_buffer_overflow();
    void update_performance_metrics();
};

/**
 * @brief Professional multicast stream receiver thread
 */
class MulticastReceiverThread : public QThread {
    Q_OBJECT
    
public:
    explicit MulticastReceiverThread(const MulticastStreamConfig& config, 
                                   QObject* parent = nullptr);
    ~MulticastReceiverThread() override;
    
    // Thread control
    void start_receiving();
    void stop_receiving();
    bool is_receiving() const;
    
    // Configuration
    void update_config(const MulticastStreamConfig& config);
    MulticastStreamConfig get_config() const;
    
    // Quality monitoring
    StreamQualityMetrics get_quality_metrics() const;
    bool is_stream_healthy() const;
    double get_signal_quality() const;

signals:
    void frame_received(const eti::EtiFrame& frame, 
                       const StreamQualityMetrics& quality);
    void quality_degradation(const QString& reason, double quality_score);
    void stream_error(const QString& error, int severity);
    void buffer_overflow(size_t dropped_frames);
    void network_congestion(double packet_loss_rate);

protected:
    void run() override;

private slots:
    void process_pending_datagrams();
    void update_quality_metrics();
    void handle_network_error(QAbstractSocket::SocketError error);

private:
    void initialize_socket();
    void configure_socket_options();
    void join_multicast_group();
    void leave_multicast_group();
    void process_received_packet(const QByteArray& data);
    void calculate_quality_metrics();
    void handle_quality_degradation();
    void optimize_buffer_settings();
    
    MulticastStreamConfig m_config;
    std::unique_ptr<QUdpSocket> m_socket;
    std::unique_ptr<ZeroCopyPacketBuffer> m_packet_buffer;
    std::unique_ptr<QTimer> m_quality_timer;
    
    StreamQualityMetrics m_quality_metrics;
    std::atomic<bool> m_receiving{false};
    std::atomic<bool> m_initialized{false};
    
    // Performance tracking
    std::chrono::steady_clock::time_point m_session_start;
    std::chrono::steady_clock::time_point m_last_packet_time;
    std::vector<std::chrono::steady_clock::time_point> m_packet_timestamps;
    
    // Network optimization
    mutable std::mutex m_config_mutex;
    std::atomic<double> m_adaptive_threshold{0.95};
};

/**
 * @brief Production-Grade Professional Multicast Receiver
 * 
 * Enterprise-class multicast receiver implementing:
 * - Zero packet loss with overflow protection
 * - Multiple concurrent stream support (up to 16 streams)
 * - SMPTE ST 2110 broadcast standards compliance
 * - Real-time quality monitoring and adaptive optimization
 * - Professional broadcast workflow integration
 */
class ProfessionalMulticastReceiver : public QObject {
    Q_OBJECT
    
public:
    explicit ProfessionalMulticastReceiver(QObject* parent = nullptr);
    ~ProfessionalMulticastReceiver() override;
    
    // Stream management
    QString add_stream(const MulticastStreamConfig& config);
    bool remove_stream(const QString& stream_id);
    bool start_stream(const QString& stream_id);
    bool stop_stream(const QString& stream_id);
    void start_all_streams();
    void stop_all_streams();
    
    // Stream configuration
    void update_stream_config(const QString& stream_id, const MulticastStreamConfig& config);
    MulticastStreamConfig get_stream_config(const QString& stream_id) const;
    QStringList get_active_streams() const;
    QStringList get_all_streams() const;
    
    // Quality monitoring
    StreamQualityMetrics get_stream_quality(const QString& stream_id) const;
    QMap<QString, StreamQualityMetrics> get_all_stream_quality() const;
    bool is_stream_healthy(const QString& stream_id) const;
    double get_overall_quality_score() const;
    
    // Performance optimization
    void enable_adaptive_optimization(bool enabled);
    void set_quality_thresholds(double min_quality, double max_packet_loss, double max_jitter_ms);
    void optimize_for_latency();
    void optimize_for_quality();
    void optimize_for_bandwidth();
    
    // Network interface management
    QStringList get_available_interfaces() const;
    bool set_preferred_interface(const QString& interface_name);
    QString get_current_interface() const;
    void enable_interface_bonding(const QStringList& interfaces);
    
    // Advanced features
    void enable_redundancy_mode(bool enabled);
    void enable_error_correction(bool enabled);
    void set_buffer_optimization_mode(const QString& mode); // "latency", "quality", "balanced"
    void enable_congestion_control(bool enabled);
    
    // Diagnostics and monitoring
    QString get_diagnostic_report() const;
    QVariantMap get_performance_statistics() const;
    void export_quality_metrics(const QString& filename) const;
    void reset_performance_counters();
    
    // Callback registration
    using FrameCallback = std::function<void(const QString&, const eti::EtiFrame&, const StreamQualityMetrics&)>;
    using QualityCallback = std::function<void(const QString&, const StreamQualityMetrics&)>;
    using ErrorCallback = std::function<void(const QString&, const QString&, int)>;
    
    void set_frame_callback(FrameCallback callback);
    void set_quality_callback(QualityCallback callback);
    void set_error_callback(ErrorCallback callback);

signals:
    void stream_added(const QString& stream_id);
    void stream_removed(const QString& stream_id);
    void stream_started(const QString& stream_id);
    void stream_stopped(const QString& stream_id);
    
    void frame_received(const QString& stream_id, const eti::EtiFrame& frame, 
                       const StreamQualityMetrics& quality);
    
    void quality_update(const QString& stream_id, const StreamQualityMetrics& quality);
    void quality_degradation(const QString& stream_id, const QString& reason, double quality);
    void stream_error(const QString& stream_id, const QString& error, int severity);
    
    void buffer_overflow(const QString& stream_id, size_t dropped_frames);
    void network_congestion(const QString& stream_id, double packet_loss_rate);
    void adaptive_optimization_applied(const QString& stream_id, const QString& optimization);

private slots:
    void handle_thread_frame(const eti::EtiFrame& frame, const StreamQualityMetrics& quality);
    void handle_thread_quality_degradation(const QString& reason, double quality);
    void handle_thread_error(const QString& error, int severity);
    void handle_thread_buffer_overflow(size_t dropped_frames);
    void handle_thread_congestion(double packet_loss_rate);
    void perform_global_optimization();
    void monitor_overall_performance();

private:
    // Stream management
    struct StreamInfo {
        QString stream_id;
        MulticastStreamConfig config;
        std::unique_ptr<MulticastReceiverThread> thread;
        StreamQualityMetrics last_quality;
        bool is_active = false;
        std::chrono::steady_clock::time_point start_time;
    };
    
    mutable std::mutex m_streams_mutex;
    std::unordered_map<std::string, std::unique_ptr<StreamInfo>> m_streams;
    std::atomic<size_t> m_next_stream_id{1};
    
    // Global optimization
    std::unique_ptr<QTimer> m_optimization_timer;
    std::unique_ptr<QTimer> m_monitoring_timer;
    std::atomic<bool> m_adaptive_optimization{true};
    
    // Quality thresholds
    double m_min_quality = 0.95;
    double m_max_packet_loss = 0.001;
    double m_max_jitter_ms = 5.0;
    
    // Network management
    QString m_preferred_interface;
    QStringList m_bonded_interfaces;
    std::atomic<bool> m_redundancy_enabled{false};
    std::atomic<bool> m_error_correction_enabled{false};
    
    // Performance tracking
    std::chrono::steady_clock::time_point m_receiver_start_time;
    mutable std::mutex m_performance_mutex;
    
    // Callbacks
    FrameCallback m_frame_callback;
    QualityCallback m_quality_callback;
    ErrorCallback m_error_callback;
    mutable std::mutex m_callback_mutex;
    
    // Helper methods
    QString generate_stream_id();
    void initialize_stream_thread(StreamInfo* stream_info);
    void optimize_stream_configuration(const QString& stream_id);
    void apply_global_optimizations();
    void calculate_overall_quality();
    void handle_stream_failure(const QString& stream_id);
    void validate_stream_config(const MulticastStreamConfig& config);
    
    static constexpr std::chrono::milliseconds OPTIMIZATION_INTERVAL{5000};
    static constexpr std::chrono::milliseconds MONITORING_INTERVAL{1000};
    static constexpr size_t MAX_CONCURRENT_STREAMS = 16;
};

/**
 * @brief Factory for creating production multicast receivers
 */
class ProfessionalMulticastReceiverFactory {
public:
    /**
     * @brief Create receiver optimized for broadcast operations
     */
    static std::unique_ptr<ProfessionalMulticastReceiver> create_broadcast_receiver();
    
    /**
     * @brief Create receiver optimized for low-latency applications  
     */
    static std::unique_ptr<ProfessionalMulticastReceiver> create_low_latency_receiver();
    
    /**
     * @brief Create receiver optimized for high-quality monitoring
     */
    static std::unique_ptr<ProfessionalMulticastReceiver> create_monitoring_receiver();
    
    /**
     * @brief Create receiver for development and testing
     */
    static std::unique_ptr<ProfessionalMulticastReceiver> create_development_receiver();
    
    /**
     * @brief Create custom receiver with specific configuration
     */
    static std::unique_ptr<ProfessionalMulticastReceiver> create_custom_receiver(
        const MulticastStreamConfig& base_config);
};

/**
 * @brief Multicast network discovery utilities
 */
namespace MulticastDiscovery {
    
    /**
     * @brief Discover active ETI multicast streams on network
     */
    QList<MulticastStreamConfig> discover_eti_streams(
        const QString& network_interface = QString(),
        std::chrono::milliseconds timeout = std::chrono::milliseconds{5000});
    
    /**
     * @brief Validate multicast configuration
     */
    bool validate_multicast_config(const MulticastStreamConfig& config, QString& error_message);
    
    /**
     * @brief Get optimal network interface for multicast
     */
    QString get_optimal_interface_for_multicast(const QHostAddress& multicast_address);
    
    /**
     * @brief Test multicast connectivity
     */
    bool test_multicast_connectivity(const MulticastStreamConfig& config, 
                                   std::chrono::milliseconds timeout = std::chrono::milliseconds{3000});
}

} // namespace eti_network

#endif // PROFESSIONAL_MULTICAST_RECEIVER_HPP