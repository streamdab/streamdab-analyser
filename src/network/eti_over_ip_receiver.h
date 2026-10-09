/**
 * @file eti_over_ip_receiver.h
 * @brief Advanced ETI-over-IP Receiver with RTP/UDP Multicast Support
 * 
 * Implements professional broadcast-grade ETI-over-IP reception with:
 * - RFC 3550 RTP protocol support for professional broadcasting
 * - Advanced packet reassembly and jitter buffer management
 * - Redundancy handling and automatic failover
 * - Real-time performance monitoring <50ms latency
 * - Network quality assessment and adaptation
 * 
 * @author Network/Stream Agent
 * @date 2025-09-21
 * @copyright Copyright (c) 2025 ETI Stream Analyser Team
 */

#ifndef ETI_OVER_IP_RECEIVER_H
#define ETI_OVER_IP_RECEIVER_H

#include <QObject>
#include <QUdpSocket>
#include <QTimer>
#include <QMutex>
#include <QQueue>
#include <QNetworkInterface>
#include <QHostAddress>
#include <memory>
#include <atomic>
#include <chrono>
#include <unordered_map>
#include <functional>

#include "../core/eti_types.hpp"

namespace eti_network {

/**
 * @brief RTP packet header structure (RFC 3550)
 */
struct RTPHeader {
    uint8_t version : 2;          // Version (always 2)
    uint8_t padding : 1;          // Padding flag
    uint8_t extension : 1;        // Extension flag
    uint8_t csrc_count : 4;       // CSRC count
    uint8_t marker : 1;           // Marker bit
    uint8_t payload_type : 7;     // Payload type
    uint16_t sequence_number;     // Sequence number
    uint32_t timestamp;           // Timestamp
    uint32_t ssrc;               // Synchronization source
    
    static constexpr size_t HEADER_SIZE = 12;
    static constexpr uint8_t ETI_PAYLOAD_TYPE = 96; // Dynamic payload type for ETI
    
    bool is_valid() const {
        return version == 2 && payload_type == ETI_PAYLOAD_TYPE;
    }
    
    void to_network_order();
    void from_network_order();
};

/**
 * @brief Redundancy mode for professional broadcast reliability
 */
enum class RedundancyMode : std::uint8_t {
    NONE,               // No redundancy
    DUAL_STREAM,        // Dual stream redundancy (main + backup)
    FEC_REDUNDANCY,     // Forward Error Correction
    TIME_DIVERSITY      // Time diversity redundancy
};

/**
 * @brief Network quality metrics for real-time assessment
 */
struct NetworkQualityMetrics {
    std::atomic<double> jitter_ms{0.0};           // Network jitter in milliseconds
    std::atomic<double> packet_loss_rate{0.0};    // Packet loss rate (0.0-1.0)
    std::atomic<double> out_of_order_rate{0.0};   // Out of order packet rate
    std::atomic<int64_t> round_trip_time_us{0};   // Round trip time in microseconds
    std::atomic<double> bandwidth_mbps{0.0};      // Current bandwidth in Mbps
    std::atomic<double> buffer_fill_level{0.0};   // Jitter buffer fill level (0.0-1.0)
    
    // Professional broadcast quality indicators
    std::atomic<bool> sync_locked{false};         // Synchronization lock status
    std::atomic<double> signal_quality{0.0};     // Signal quality index (0.0-1.0)
    std::atomic<int> continuity_errors{0};       // Continuity counter errors
    std::atomic<int> crc_errors{0};              // CRC errors detected
    
    // Custom constructors to handle atomic members
    NetworkQualityMetrics() = default;
    
    NetworkQualityMetrics(const NetworkQualityMetrics& other) {
        jitter_ms = other.jitter_ms.load();
        packet_loss_rate = other.packet_loss_rate.load();
        out_of_order_rate = other.out_of_order_rate.load();
        round_trip_time_us = other.round_trip_time_us.load();
        bandwidth_mbps = other.bandwidth_mbps.load();
        buffer_fill_level = other.buffer_fill_level.load();
        sync_locked = other.sync_locked.load();
        signal_quality = other.signal_quality.load();
        continuity_errors = other.continuity_errors.load();
        crc_errors = other.crc_errors.load();
    }
    
    NetworkQualityMetrics& operator=(const NetworkQualityMetrics& other) {
        if (this != &other) {
            jitter_ms = other.jitter_ms.load();
            packet_loss_rate = other.packet_loss_rate.load();
            out_of_order_rate = other.out_of_order_rate.load();
            round_trip_time_us = other.round_trip_time_us.load();
            bandwidth_mbps = other.bandwidth_mbps.load();
            buffer_fill_level = other.buffer_fill_level.load();
            sync_locked = other.sync_locked.load();
            signal_quality = other.signal_quality.load();
            continuity_errors = other.continuity_errors.load();
            crc_errors = other.crc_errors.load();
        }
        return *this;
    }
    
    void reset() {
        jitter_ms = 0.0;
        packet_loss_rate = 0.0;
        out_of_order_rate = 0.0;
        round_trip_time_us = 0;
        bandwidth_mbps = 0.0;
        buffer_fill_level = 0.0;
        sync_locked = false;
        signal_quality = 0.0;
        continuity_errors = 0;
        crc_errors = 0;
    }
};

/**
 * @brief Jitter buffer for packet ordering and timing recovery
 */
class JitterBuffer {
public:
    explicit JitterBuffer(size_t max_buffer_size = 100);
    ~JitterBuffer() = default;
    
    // Packet management
    void add_packet(const QByteArray& packet, uint16_t sequence_number, uint32_t timestamp);
    QByteArray get_next_packet();
    bool has_packet_ready() const;
    
    // Buffer control
    void set_target_delay(std::chrono::milliseconds delay);
    void set_adaptive_mode(bool enabled);
    void flush();
    
    // Statistics
    size_t get_buffer_size() const;
    double get_fill_level() const;
    std::chrono::milliseconds get_current_delay() const;
    size_t get_dropped_packets() const;
    size_t get_duplicate_packets() const;
    
private:
    struct BufferedPacket {
        QByteArray data;
        uint16_t sequence_number;
        uint32_t timestamp;
        std::chrono::steady_clock::time_point arrival_time;
        bool is_valid = true;
    };
    
    void cleanup_old_packets();
    void update_adaptive_delay();
    bool is_packet_late(const BufferedPacket& packet) const;
    
    mutable QMutex m_mutex;
    std::unordered_map<uint16_t, BufferedPacket> m_buffer;
    uint16_t m_next_expected_sequence = 0;
    uint32_t m_base_timestamp = 0;
    
    size_t m_max_buffer_size;
    std::chrono::milliseconds m_target_delay{50};
    bool m_adaptive_mode = true;
    
    // Statistics
    std::atomic<size_t> m_dropped_packets{0};
    std::atomic<size_t> m_duplicate_packets{0};
    std::atomic<size_t> m_out_of_order_packets{0};
    
    std::chrono::steady_clock::time_point m_last_cleanup;
    static constexpr std::chrono::milliseconds CLEANUP_INTERVAL{1000};
};

/**
 * @brief RTP session manager for professional broadcast operations
 */
class RTPSession {
public:
    explicit RTPSession(uint32_t ssrc = 0);
    ~RTPSession() = default;
    
    // Packet processing
    bool process_rtp_packet(const QByteArray& raw_packet, QByteArray& payload);
    QByteArray create_rtp_packet(const QByteArray& payload, bool marker = false);
    
    // Session management
    void set_payload_type(uint8_t payload_type);
    void set_ssrc(uint32_t ssrc);
    void reset_sequence();
    
    // Statistics
    uint16_t get_sequence_number() const;
    uint32_t get_timestamp() const;
    uint32_t get_ssrc() const;
    size_t get_packets_sent() const;
    size_t get_packets_received() const;
    size_t get_bytes_sent() const;
    size_t get_bytes_received() const;
    
    // Quality metrics
    NetworkQualityMetrics get_quality_metrics() const;
    void update_quality_metrics(const QByteArray& packet);
    
private:
    void update_statistics(const RTPHeader& header, size_t payload_size, bool outgoing);
    void calculate_jitter(uint32_t timestamp);
    void detect_packet_loss(uint16_t sequence_number);
    static uint32_t generate_random_ssrc();
    
    uint32_t m_ssrc;
    uint8_t m_payload_type = RTPHeader::ETI_PAYLOAD_TYPE;
    uint16_t m_sequence_number = 0;
    uint32_t m_timestamp = 0;
    
    // Statistics
    std::atomic<size_t> m_packets_sent{0};
    std::atomic<size_t> m_packets_received{0};
    std::atomic<size_t> m_bytes_sent{0};
    std::atomic<size_t> m_bytes_received{0};
    
    // Quality tracking
    NetworkQualityMetrics m_quality_metrics;
    uint16_t m_last_sequence_received = 0;
    uint16_t m_last_sequence_sent = 0;
    uint32_t m_last_timestamp_received = 0;
    uint32_t m_last_timestamp = 0;
    std::atomic<size_t> m_packets_lost{0};
    std::chrono::steady_clock::time_point m_last_packet_time;
    
    // Jitter calculation (RFC 3550)
    double m_jitter_accumulator = 0.0;
    static constexpr double JITTER_ALPHA = 0.125; // Smoothing factor
};

/**
 * @brief Redundancy manager for professional broadcast reliability
 */
class RedundancyManager {
public:
    explicit RedundancyManager(RedundancyMode mode = RedundancyMode::DUAL_STREAM);
    ~RedundancyManager() = default;
    
    // Configuration
    void set_redundancy_mode(RedundancyMode mode);
    void add_backup_stream(const QString& multicast_address, quint16 port);
    void set_failover_threshold(double packet_loss_threshold);
    void set_recovery_threshold(double recovery_threshold);
    
    // Stream management
    bool process_primary_stream(const QByteArray& data);
    bool process_backup_stream(const QByteArray& data);
    QByteArray get_best_frame();
    bool has_frame_ready() const;
    
    // Status monitoring
    bool is_primary_healthy() const;
    bool is_backup_healthy() const;
    bool is_failover_active() const;
    RedundancyMode get_current_mode() const;
    
    // Statistics
    size_t get_primary_packets() const;
    size_t get_backup_packets() const;
    size_t get_failover_count() const;
    double get_primary_quality() const;
    double get_backup_quality() const;
    
private:
    struct StreamInfo {
        QQueue<QByteArray> frame_buffer;
        std::chrono::steady_clock::time_point last_packet_time;
        std::chrono::steady_clock::time_point last_seen;
        QString address;
        quint16 port = 0;
        size_t packet_count = 0;
        size_t error_count = 0;
        double quality_score = 1.0;
        bool is_healthy = true;
        bool is_active = false;
    };
    
    void update_stream_quality(StreamInfo& stream, bool packet_ok);
    void check_failover_conditions();
    QByteArray select_best_frame();
    
    RedundancyMode m_mode;
    StreamInfo m_primary_stream;
    StreamInfo m_backup_stream;
    QList<StreamInfo> m_backup_streams;
    
    double m_failover_threshold = 0.05;  // 5% packet loss triggers failover
    double m_recovery_threshold = 0.01;  // 1% packet loss for recovery
    bool m_failover_active = false;
    
    std::atomic<size_t> m_failover_count{0};
    mutable QMutex m_mutex;
};

/**
 * @brief Advanced ETI-over-IP Receiver with Professional Broadcasting Features
 * 
 * Comprehensive ETI-over-IP receiver implementing:
 * - RFC 3550 RTP protocol support
 * - Advanced jitter buffer management
 * - Redundancy and automatic failover
 * - Real-time quality monitoring
 * - Professional broadcast integration
 */
class EtiOverIpReceiver : public QObject {
    Q_OBJECT
    
public:
    explicit EtiOverIpReceiver(QObject *parent = nullptr);
    ~EtiOverIpReceiver();
    
    // Connection management
    bool startListening(const QString& multicastAddress, quint16 port);
    bool startListening(const QString& multicastAddress, quint16 port, 
                       const QString& networkInterface);
    void stopListening();
    bool isListening() const;
    
    // Advanced configuration
    void setBufferSize(qint64 bufferSizeMs);
    void setJitterBuffer(qint64 jitterMs);
    void setRedundancyMode(RedundancyMode mode);
    void addBackupStream(const QString& multicastAddress, quint16 port);
    void setAdaptiveBuffering(bool enabled);
    void setQualityThresholds(double minSignalQuality, double maxPacketLoss, double maxJitter);
    
    // Professional broadcast features
    void enableProfessionalMode(bool enabled);
    void setTimestampValidation(bool enabled);
    void setContinuityChecking(bool enabled);
    void setErrorCorrection(bool enabled);
    
    // Real-time performance
    void setMaxLatency(std::chrono::milliseconds maxLatency);
    void setPriorityMode(bool highPriority);
    void setThreadAffinity(int cpuCore);
    
    // Network interface management
    QStringList getAvailableInterfaces() const;
    bool setNetworkInterface(const QString& interfaceName);
    QString getCurrentInterface() const;
    void enableInterfaceMonitoring(bool enabled);
    
    // Quality monitoring
    NetworkQualityMetrics getQualityMetrics() const;
    double getSignalQuality() const;
    double getPacketLossRate() const;
    double getJitter() const;
    std::chrono::microseconds getCurrentLatency() const;
    bool isQualityAcceptable() const;
    
    // Statistics and diagnostics
    size_t getTotalPacketsReceived() const;
    size_t getTotalFramesProcessed() const;
    size_t getErrorCount() const;
    double getCurrentBandwidth() const;
    QString getDiagnosticInfo() const;
    
    // Callback registration for real-time processing
    using FrameCallback = std::function<void(const eti::EtiFrame&, const NetworkQualityMetrics&)>;
    using ErrorCallback = std::function<void(const QString&, int severity)>;
    using QualityCallback = std::function<void(const NetworkQualityMetrics&)>;
    
    void setFrameCallback(FrameCallback callback);
    void setErrorCallback(ErrorCallback callback);
    void setQualityCallback(QualityCallback callback);

signals:
    void etiFrameReceived(const eti::EtiFrame& frame, const NetworkQualityMetrics& quality);
    void networkError(const QString& error, int severity);
    void connectionStatusChanged(bool connected);
    void qualityMetricsUpdated(const NetworkQualityMetrics& metrics);
    void redundancyStatusChanged(bool failoverActive, const QString& activeStream);
    void bufferStatusChanged(double fillLevel, std::chrono::milliseconds currentDelay);
    void interfaceChanged(const QString& newInterface, const QString& reason);
    
    // Professional broadcast signals
    void syncLockAchieved();
    void syncLockLost();
    void continuityError(int errorCount);
    void qualityThresholdViolated(const QString& parameter, double currentValue, double threshold);

private slots:
    void handleIncomingData();
    void handleNetworkError(QAbstractSocket::SocketError error);
    void updateQualityMetrics();
    void checkQualityThresholds();
    void handleInterfaceChanged();
    void performMaintenance();

private:
    // Initialization and cleanup
    void initializeReceiver();
    void cleanupReceiver();
    bool setupSocket();  // Changed return type to bool
    void setupTimers();
    void connectSignals();
    bool joinMulticastGroup();
    void leaveMulticastGroup();
    
    // Packet processing
    void processIncomingPacket(const QByteArray& packet, const QHostAddress& sender);
    bool validateRTPPacket(const QByteArray& packet);
    void reconstructETIFrame(const QByteArray& rtpPayload);
    void updateNetworkStatistics(const QByteArray& packet);
    
    // Quality management
    void updateQualityAssessment();
    void adaptBufferSettings();
    void handleQualityDegradation();
    void performAutomaticRecovery();
    
    // Network interface management
    void scanNetworkInterfaces() const;  // Made const for ETSI compliance validation
    void selectOptimalInterface();
    void handleInterfaceFailure();
    
    // Core components
    std::unique_ptr<QUdpSocket> m_socket;
    std::unique_ptr<QTimer> m_qualityTimer;
    std::unique_ptr<QTimer> m_maintenanceTimer;
    std::unique_ptr<QTimer> m_interfaceTimer;
    
    // Advanced processing components
    std::unique_ptr<RTPSession> m_rtpSession;
    std::unique_ptr<JitterBuffer> m_jitterBuffer;
    std::unique_ptr<RedundancyManager> m_redundancyManager;
    
    // Configuration
    QString m_multicastAddress;
    quint16 m_port = 0;
    QString m_networkInterface;
    qint64 m_bufferSizeMs = 100;
    qint64 m_jitterBufferMs = 50;
    RedundancyMode m_redundancyMode = RedundancyMode::NONE;
    
    // Quality thresholds
    double m_minSignalQuality = 0.8;
    double m_maxPacketLoss = 0.01;  // 1%
    double m_maxJitter = 10.0;      // 10ms
    std::chrono::milliseconds m_maxLatency{50};
    
    // Professional broadcast settings
    bool m_professionalMode = false;
    bool m_timestampValidation = true;
    bool m_continuityChecking = true;
    bool m_errorCorrection = true;
    bool m_adaptiveBuffering = true;
    bool m_highPriority = false;
    
    // State management
    std::atomic<bool> m_listening{false};
    std::atomic<bool> m_connected{false};
    std::atomic<bool> m_syncLocked{false};
    
    // Quality metrics
    NetworkQualityMetrics m_qualityMetrics;
    mutable QMutex m_qualityMutex;
    
    // Statistics
    std::atomic<size_t> m_totalPacketsReceived{0};
    std::atomic<size_t> m_totalFramesProcessed{0};
    std::atomic<size_t> m_totalErrors{0};
    std::atomic<double> m_currentBandwidth{0.0};
    
    // Callbacks
    FrameCallback m_frameCallback;
    ErrorCallback m_errorCallback;
    QualityCallback m_qualityCallback;
    mutable QMutex m_callbackMutex;
    
    // Network interface monitoring
    mutable QList<QNetworkInterface> m_availableInterfaces;  // Mutable for const method access
    QString m_currentInterface;
    bool m_interfaceMonitoring = false;
    
    // Error handling
    QStringList m_errorHistory;
    mutable QMutex m_errorMutex;
    static constexpr size_t MAX_ERROR_HISTORY = 100;
    
    // Performance optimization
    static constexpr size_t PACKET_BUFFER_SIZE = 65536;
    static constexpr std::chrono::milliseconds QUALITY_UPDATE_INTERVAL{1000};
    static constexpr std::chrono::milliseconds MAINTENANCE_INTERVAL{5000};
    static constexpr std::chrono::milliseconds INTERFACE_CHECK_INTERVAL{10000};
};

/**
 * @brief Factory for creating specialized ETI-over-IP receivers
 */
class EtiOverIpReceiverFactory {
public:
    /**
     * @brief Create receiver for professional broadcast operations
     */
    static std::unique_ptr<EtiOverIpReceiver> createBroadcastReceiver(
        const QString& multicastAddress, quint16 port);
    
    /**
     * @brief Create low-latency receiver for real-time applications
     */
    static std::unique_ptr<EtiOverIpReceiver> createLowLatencyReceiver(
        const QString& multicastAddress, quint16 port, 
        std::chrono::milliseconds maxLatency = std::chrono::milliseconds{20});
    
    /**
     * @brief Create redundant receiver with automatic failover
     */
    static std::unique_ptr<EtiOverIpReceiver> createRedundantReceiver(
        const QString& primaryAddress, const QString& backupAddress, quint16 port);
    
    /**
     * @brief Create high-quality receiver with advanced error correction
     */
    static std::unique_ptr<EtiOverIpReceiver> createHighQualityReceiver(
        const QString& multicastAddress, quint16 port, 
        double minSignalQuality = 0.95);
};

} // namespace eti_network

#endif // ETI_OVER_IP_RECEIVER_H