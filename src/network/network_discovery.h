/**
 * @file network_discovery.h
 * @brief Professional Network Discovery for ETI Streams and Interfaces
 * 
 * Implements comprehensive network discovery and monitoring for:
 * - ETI-over-IP multicast stream detection
 * - Network interface discovery and monitoring
 * - Broadcast equipment discovery (SAP/SDP)
 * - Professional broadcast network management
 * - Real-time network topology monitoring
 * 
 * @author Network/Stream Agent
 * @date 2025-09-21
 * @copyright Copyright (c) 2025 ETI Stream Analyser Team
 */

#ifndef NETWORK_DISCOVERY_H
#define NETWORK_DISCOVERY_H

#include <QObject>
#include <QNetworkInterface>
#include <QUdpSocket>
#include <QTimer>
#include <QHostAddress>
#include <QNetworkDatagram>
#include <QJsonObject>
#include <QJsonDocument>
#include <QMutex>
#include <memory>
#include <atomic>
#include <chrono>
#include <unordered_map>
#include <functional>

#include "../core/eti_types.hpp"

namespace eti_network {

/**
 * @brief Discovery protocol enumeration for different detection methods
 */
enum class DiscoveryProtocol {
    MULTICAST_SCAN,         // Scan multicast groups for ETI streams
    SAP_SDP,               // Session Announcement Protocol / Session Description Protocol
    BROADCAST_DISCOVERY,    // Broadcast ping-based discovery
    UPNP_DISCOVERY,        // UPnP-based equipment discovery
    MDNS_DISCOVERY,        // mDNS/Bonjour discovery
    MANUAL_PROBE,          // Manual IP range scanning
    ALL_PROTOCOLS          // Use all available protocols
};

/**
 * @brief ETI stream information from network discovery
 */
struct ETIStreamInfo {
    QString stream_id;                    // Unique stream identifier
    QString name;                         // Human-readable stream name
    QString description;                  // Stream description
    
    // Network configuration
    QHostAddress multicast_address;      // Multicast address
    quint16 port = 0;                    // UDP port
    QString network_interface;           // Network interface name
    QString source_address;              // Source equipment address
    
    // Stream characteristics
    QString encoding_format;             // ETI-NI, ETI-LI, EDI
    double bit_rate_kbps = 0.0;          // Stream bit rate
    double frame_rate_fps = 0.0;         // Frame rate
    size_t mtu_size = 1500;              // Maximum transmission unit
    
    // Quality metrics
    double signal_strength = 0.0;        // Signal strength (0.0-1.0)
    double packet_loss_rate = 0.0;       // Packet loss rate
    std::chrono::milliseconds latency{0}; // Network latency
    std::chrono::steady_clock::time_point last_seen; // Last activity time
    
    // Service information
    QString ensemble_name;               // DAB ensemble name
    quint16 ensemble_id = 0;             // Ensemble identifier
    QString country_code;                // Country code
    size_t service_count = 0;            // Number of services
    QStringList service_names;           // Service names list
    
    // Discovery metadata
    DiscoveryProtocol discovery_method;  // How stream was discovered
    QString equipment_manufacturer;      // Equipment manufacturer
    QString equipment_model;             // Equipment model
    QString equipment_version;           // Equipment version
    QJsonObject custom_metadata;        // Custom metadata
    
    // Availability status
    bool is_active = false;              // Stream currently active
    bool is_accessible = false;          // Stream accessible from this host
    bool requires_authentication = false; // Authentication required
    QString authentication_method;       // Authentication method if required
    
    bool is_valid() const {
        return !stream_id.isEmpty() && 
               multicast_address.isMulticast() && 
               port > 0;
    }
    
    bool is_recent(std::chrono::seconds timeout = std::chrono::seconds{30}) const {
        const auto now = std::chrono::steady_clock::now();
        return (now - last_seen) < timeout;
    }
    
    QString to_url() const {
        return QString("udp://%1:%2").arg(multicast_address.toString()).arg(port);
    }
};

/**
 * @brief Network interface information with broadcast capabilities
 */
struct NetworkInterfaceInfo {
    QString name;                        // Interface name (e.g., "eth0")
    QString display_name;                // Human-readable name
    QString description;                 // Interface description
    QString hardware_address;           // MAC address
    
    // Network configuration
    QList<QHostAddress> ip_addresses;   // IP addresses
    QList<QHostAddress> netmasks;       // Network masks
    QList<QHostAddress> broadcast_addresses; // Broadcast addresses
    QString gateway_address;             // Default gateway
    QStringList dns_servers;            // DNS server addresses
    
    // Interface capabilities
    bool supports_multicast = false;    // Multicast support
    bool supports_broadcast = false;    // Broadcast support
    bool is_up = false;                 // Interface is up
    bool is_running = false;            // Interface is running
    bool is_wireless = false;           // Wireless interface
    bool has_carrier = false;           // Physical carrier detected
    
    // Performance metrics
    quint64 bytes_received = 0;         // Bytes received
    quint64 bytes_sent = 0;             // Bytes sent
    quint64 packets_received = 0;       // Packets received
    quint64 packets_sent = 0;           // Packets sent
    quint64 errors_received = 0;        // Receive errors
    quint64 errors_sent = 0;            // Send errors
    
    // Quality metrics
    double bandwidth_mbps = 0.0;        // Available bandwidth
    std::chrono::milliseconds latency{0}; // Interface latency
    double packet_loss_rate = 0.0;      // Packet loss rate
    double utilization = 0.0;           // Interface utilization (0.0-1.0)
    
    // Discovery status
    std::chrono::steady_clock::time_point last_updated; // Last update time
    bool discovery_enabled = false;     // Discovery enabled on interface
    size_t discovered_streams = 0;      // Number of discovered streams
    
    bool is_suitable_for_multicast() const {
        return supports_multicast && is_up && is_running && has_carrier;
    }
    
    bool is_suitable_for_discovery() const {
        return is_suitable_for_multicast() && !ip_addresses.isEmpty();
    }
    
    QHostAddress get_primary_ip() const {
        for (const auto& addr : ip_addresses) {
            if (addr.protocol() == QAbstractSocket::IPv4Protocol && 
                !addr.isLoopback()) {
                return addr;
            }
        }
        return QHostAddress();
    }
};

/**
 * @brief Multicast group scanner for ETI stream detection
 */
class MulticastScanner : public QObject {
    Q_OBJECT
    
public:
    explicit MulticastScanner(QObject* parent = nullptr);
    ~MulticastScanner();
    
    // Scanning control
    void startScanning(const QStringList& interfaces = QStringList());
    void stopScanning();
    bool isScanning() const;
    
    // Configuration
    void setMulticastRanges(const QStringList& ranges);
    void setPortRanges(const QList<QPair<quint16, quint16>>& ranges);
    void setScanInterval(std::chrono::milliseconds interval);
    void setScanTimeout(std::chrono::milliseconds timeout);
    
    // Stream validation
    void enableStreamValidation(bool enabled);
    void setValidationTimeout(std::chrono::milliseconds timeout);
    void setMinimumFrameRate(double minFPS);
    
    // Results
    QList<ETIStreamInfo> getDiscoveredStreams() const;
    QList<ETIStreamInfo> getActiveStreams() const;
    size_t getStreamCount() const;
    
signals:
    void streamDiscovered(const ETIStreamInfo& streamInfo);
    void streamLost(const QString& streamId);
    void streamUpdated(const ETIStreamInfo& streamInfo);
    void scanningProgress(int percentage);
    void scanningCompleted();
    
private slots:
    void performScan();
    void validateStreams();
    void cleanupInactiveStreams();
    
private:
    void scanMulticastRange(const QString& baseAddress, quint16 startPort, quint16 endPort);
    void probeMulticastAddress(const QHostAddress& address, quint16 port);
    bool validateETIStream(const QHostAddress& address, quint16 port);
    ETIStreamInfo analyzeETIStream(const QHostAddress& address, quint16 port);
    
    std::unique_ptr<QTimer> m_scanTimer;
    std::unique_ptr<QTimer> m_validationTimer;
    std::unique_ptr<QTimer> m_cleanupTimer;
    
    QStringList m_multicastRanges;
    QList<QPair<quint16, quint16>> m_portRanges;
    std::chrono::milliseconds m_scanInterval{5000};
    std::chrono::milliseconds m_scanTimeout{1000};
    std::chrono::milliseconds m_validationTimeout{2000};
    
    mutable QMutex m_streamsMutex;
    std::unordered_map<QString, ETIStreamInfo> m_discoveredStreams;
    
    std::atomic<bool> m_scanning{false};
    bool m_streamValidation = true;
    double m_minimumFrameRate = 50.0;
    
    static constexpr std::chrono::seconds STREAM_TIMEOUT{60};
    static constexpr std::chrono::milliseconds VALIDATION_INTERVAL{10000};
    static constexpr std::chrono::milliseconds CLEANUP_INTERVAL{30000};
};

/**
 * @brief SAP/SDP announcement monitor for professional broadcast discovery
 */
class BroadcastAnnouncer : public QObject {
    Q_OBJECT
    
public:
    explicit BroadcastAnnouncer(QObject* parent = nullptr);
    ~BroadcastAnnouncer();
    
    // Monitoring control
    void startMonitoring();
    void stopMonitoring();
    bool isMonitoring() const;
    
    // Configuration
    void setSAPAddress(const QHostAddress& address, quint16 port = 9875);
    void enableSDPParsing(bool enabled);
    void setAnnouncementTimeout(std::chrono::seconds timeout);
    
    // Announcement management
    void announceStream(const ETIStreamInfo& streamInfo);
    void withdrawAnnouncement(const QString& streamId);
    QList<ETIStreamInfo> getAnnouncedStreams() const;
    
signals:
    void streamAnnounced(const ETIStreamInfo& streamInfo);
    void streamWithdrawn(const QString& streamId);
    void announcementUpdated(const ETIStreamInfo& streamInfo);
    
private slots:
    void handleSAPMessage();
    void cleanupExpiredAnnouncements();
    
private:
    void parseSAPPacket(const QByteArray& packet, const QHostAddress& sender);
    void parseSAPAnnouncement(const QString& sdpContent, const QHostAddress& sender, bool isAnnouncement, quint16 messageId);
    ETIStreamInfo parseSDPSession(const QString& sdpContent, const QHostAddress& sender);
    void processSessionAnnouncement(const ETIStreamInfo& streamInfo);
    void processSessionDeletion(const QString& streamId);
    
    std::unique_ptr<QUdpSocket> m_sapSocket;
    std::unique_ptr<QTimer> m_cleanupTimer;
    
    QHostAddress m_sapAddress{QHostAddress("224.2.127.254")};
    quint16 m_sapPort = 9875;
    std::chrono::seconds m_announcementTimeout{300}; // 5 minutes
    
    mutable QMutex m_announcementsMutex;
    std::unordered_map<QString, ETIStreamInfo> m_announcements;
    std::unordered_map<QString, std::chrono::steady_clock::time_point> m_lastSeen;
    
    std::atomic<bool> m_monitoring{false};
    bool m_sdpParsing = true;
    
    static constexpr std::chrono::milliseconds CLEANUP_INTERVAL{60000};
};

/**
 * @brief Network interface monitor for dynamic topology management
 */
class NetworkInterfaceMonitor : public QObject {
    Q_OBJECT
    
public:
    explicit NetworkInterfaceMonitor(QObject* parent = nullptr);
    ~NetworkInterfaceMonitor();
    
    // Monitoring control
    void startMonitoring();
    void stopMonitoring();
    bool isMonitoring() const;
    
    // Configuration
    void setMonitoringInterval(std::chrono::milliseconds interval);
    void enablePerformanceMonitoring(bool enabled);
    void enableQualityAssessment(bool enabled);
    
    // Interface management
    QList<NetworkInterfaceInfo> getAvailableInterfaces() const;
    QList<NetworkInterfaceInfo> getMulticastCapableInterfaces() const;
    NetworkInterfaceInfo getInterfaceInfo(const QString& name) const;
    QString selectOptimalInterface(const QHostAddress& targetAddress) const;
    
    // Performance monitoring
    void updateInterfaceStatistics();
    void assessInterfaceQuality();
    QStringList getInterfacePerformanceReport() const;
    
signals:
    void interfaceAdded(const NetworkInterfaceInfo& interfaceInfo);
    void interfaceRemoved(const QString& interfaceName);
    void interfaceUpdated(const NetworkInterfaceInfo& interfaceInfo);
    void interfaceQualityChanged(const QString& interfaceName, double quality);
    void optimalInterfaceChanged(const QString& interfaceName);
    
private slots:
    void scanInterfaces();
    void updateStatistics();
    void assessQuality();
    
private:
    void detectInterfaceChanges();
    NetworkInterfaceInfo analyzeInterface(const QNetworkInterface& interface);
    void updateInterfaceStatistics(NetworkInterfaceInfo& info);
    double calculateInterfaceQuality(const NetworkInterfaceInfo& info);
    void selectNewOptimalInterface();
    
    std::unique_ptr<QTimer> m_scanTimer;
    std::unique_ptr<QTimer> m_statisticsTimer;
    std::unique_ptr<QTimer> m_qualityTimer;
    
    mutable QMutex m_interfacesMutex;
    std::unordered_map<QString, NetworkInterfaceInfo> m_interfaces;
    QString m_optimalInterface;
    
    std::chrono::milliseconds m_monitoringInterval{5000};
    bool m_performanceMonitoring = true;
    bool m_qualityAssessment = true;
    std::atomic<bool> m_monitoring{false};
    
    static constexpr std::chrono::milliseconds STATISTICS_INTERVAL{1000};
    static constexpr std::chrono::milliseconds QUALITY_INTERVAL{10000};
};

/**
 * @brief Professional Network Discovery Manager
 * 
 * Comprehensive network discovery system implementing:
 * - Multi-protocol ETI stream discovery
 * - Network interface monitoring and management
 * - Professional broadcast equipment detection
 * - Real-time network topology monitoring
 * - Automatic failover and redundancy management
 */
class NetworkDiscovery : public QObject {
    Q_OBJECT
    
public:
    explicit NetworkDiscovery(QObject* parent = nullptr);
    ~NetworkDiscovery();
    
    // Discovery control
    void startDiscovery();
    void stopDiscovery();
    bool isDiscoveryActive() const;
    
    // Configuration
    void setDiscoveryProtocols(const QList<DiscoveryProtocol>& protocols);
    void enableProtocol(DiscoveryProtocol protocol, bool enabled);
    void setDiscoveryInterval(std::chrono::milliseconds interval);
    void setNetworkInterfaces(const QStringList& interfaces);
    
    // Stream discovery
    void scanForETIStreams();
    void scanSpecificRange(const QString& multicastRange, quint16 startPort, quint16 endPort);
    void addManualStream(const QString& multicastAddress, quint16 port);
    void removeStream(const QString& streamId);
    
    // Results access
    QList<ETIStreamInfo> getAvailableStreams() const;
    QList<ETIStreamInfo> getActiveStreams() const;
    QList<ETIStreamInfo> getStreamsByProtocol(DiscoveryProtocol protocol) const;
    ETIStreamInfo getStreamInfo(const QString& streamId) const;
    size_t getStreamCount() const;
    
    // Network interface management
    QList<NetworkInterfaceInfo> getNetworkInterfaces() const;
    NetworkInterfaceInfo getOptimalInterface(const QHostAddress& targetAddress) const;
    void setPreferredInterface(const QString& interfaceName);
    QString getPreferredInterface() const;
    
    // Quality and monitoring
    void enableNetworkQualityMonitoring(bool enabled);
    void setQualityThresholds(double minSignalStrength, double maxPacketLoss, 
                             std::chrono::milliseconds maxLatency);
    QStringList getNetworkDiagnostics() const;
    QString getDiscoveryReport() const;
    
    // Professional broadcast features
    void enableBroadcastDiscovery(bool enabled);
    void setBroadcastEquipmentFilter(const QStringList& manufacturers);
    void enableRedundancyDetection(bool enabled);
    void configureFailoverSettings(double qualityThreshold, std::chrono::seconds timeout);
    
    // Callback registration
    using StreamCallback = std::function<void(const ETIStreamInfo&)>;
    using InterfaceCallback = std::function<void(const NetworkInterfaceInfo&)>;
    using ErrorCallback = std::function<void(const QString&)>;
    
    void setStreamDiscoveryCallback(StreamCallback callback);
    void setInterfaceChangeCallback(InterfaceCallback callback);
    void setErrorCallback(ErrorCallback callback);

signals:
    void streamDiscovered(const ETIStreamInfo& streamInfo);
    void streamLost(const QString& streamId);
    void streamUpdated(const ETIStreamInfo& streamInfo);
    void networkInterfaceAdded(const NetworkInterfaceInfo& interfaceInfo);
    void networkInterfaceRemoved(const QString& interfaceName);
    void networkInterfaceChanged(const QString& interfaceName);
    void discoveryStarted();
    void discoveryStopped();
    void discoveryError(const QString& error);
    void qualityThresholdViolated(const QString& streamId, const QString& parameter, 
                                 double currentValue, double threshold);
    void redundancyStatusChanged(const QString& streamId, bool redundancyAvailable);
    void failoverRecommended(const QString& currentStreamId, const QString& backupStreamId);

private slots:
    void handleMulticastDiscovery(const ETIStreamInfo& streamInfo);
    void handleBroadcastAnnouncement(const ETIStreamInfo& streamInfo);
    void handleInterfaceChange(const NetworkInterfaceInfo& interfaceInfo);
    void performPeriodicDiscovery();
    void validateDiscoveredStreams();
    void updateStreamQuality();
    void checkRedundancyStatus();
    void discoveryProgress(int percentage);
    void discoveryCompleted();

private:
    // Initialization
    void initializeDiscovery();
    void cleanupDiscovery();
    void setupDiscoveryProtocols();
    void cleanupDiscoveryProtocols();
    
    // Stream management
    void addDiscoveredStream(const ETIStreamInfo& streamInfo);
    void updateStreamInfo(const ETIStreamInfo& streamInfo);
    void removeInactiveStreams();
    void validateStreamAccessibility(ETIStreamInfo& streamInfo);
    
    // Quality monitoring
    void assessStreamQuality(ETIStreamInfo& streamInfo);
    void checkQualityThresholds(const ETIStreamInfo& streamInfo);
    void updateNetworkMetrics();
    
    // Redundancy management
    void detectRedundantStreams();
    void updateRedundancyStatus();
    void recommendFailover();
    
    // Discovery components
    std::unique_ptr<MulticastScanner> m_multicastScanner;
    std::unique_ptr<BroadcastAnnouncer> m_broadcastAnnouncer;
    std::unique_ptr<NetworkInterfaceMonitor> m_interfaceMonitor;
    
    // Timers
    std::unique_ptr<QTimer> m_discoveryTimer;
    std::unique_ptr<QTimer> m_validationTimer;
    std::unique_ptr<QTimer> m_qualityTimer;
    std::unique_ptr<QTimer> m_redundancyTimer;
    
    // Configuration
    QList<DiscoveryProtocol> m_enabledProtocols;
    QStringList m_networkInterfaces;
    QString m_preferredInterface;
    std::chrono::milliseconds m_discoveryInterval{10000};
    
    // Quality thresholds
    double m_minSignalStrength = 0.5;
    double m_maxPacketLoss = 0.05;
    std::chrono::milliseconds m_maxLatency{100};
    
    // Stream storage
    mutable QMutex m_streamsMutex;
    std::unordered_map<QString, ETIStreamInfo> m_discoveredStreams;
    std::unordered_map<QString, QStringList> m_redundantStreams; // streamId -> backup streamIds
    
    // State management
    std::atomic<bool> m_discoveryActive{false};
    bool m_networkQualityMonitoring = true;
    bool m_broadcastDiscovery = true;
    bool m_redundancyDetection = true;
    
    // Professional broadcast settings
    QStringList m_equipmentFilter;
    double m_failoverQualityThreshold = 0.7;
    std::chrono::seconds m_failoverTimeout{30};
    
    // Callbacks
    StreamCallback m_streamCallback;
    InterfaceCallback m_interfaceCallback;
    ErrorCallback m_errorCallback;
    mutable QMutex m_callbackMutex;
    
    // Statistics
    std::chrono::steady_clock::time_point m_discoveryStartTime;
    std::chrono::steady_clock::time_point m_lastDiscoveryStart;
    size_t m_totalStreamsDiscovered = 0;
    size_t m_totalDiscoveryCycles = 0;
    size_t m_activeDiscoveryProtocols = 0;
    int m_discoveryProgress = 0;
    
    static constexpr std::chrono::milliseconds VALIDATION_INTERVAL{30000};
    static constexpr std::chrono::milliseconds QUALITY_UPDATE_INTERVAL{5000};
    static constexpr std::chrono::milliseconds REDUNDANCY_CHECK_INTERVAL{15000};
};

} // namespace eti_network

#endif // NETWORK_DISCOVERY_H