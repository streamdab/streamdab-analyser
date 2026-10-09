/**
 * @file sap_sdp_discovery_engine.hpp
 * @brief Professional SAP/SDP Network Discovery Engine
 * 
 * Production-grade broadcast discovery system implementing:
 * - SAP (Session Announcement Protocol) RFC 2974 compliance
 * - SDP (Session Description Protocol) RFC 4566 parsing
 * - Automatic ETI stream discovery on broadcast networks
 * - Real-time service directory with metadata extraction
 * - Professional broadcast workflow integration
 * - Multi-interface discovery with redundancy support
 * 
 * @author Network/Stream Agent - Phase 3 Production Platform
 * @date 2025-09-28
 * @copyright Copyright (c) 2025 ETI Stream Analyser Team
 */

#ifndef SAP_SDP_DISCOVERY_ENGINE_HPP
#define SAP_SDP_DISCOVERY_ENGINE_HPP

#include <QObject>
#include <QUdpSocket>
#include <QNetworkInterface>
#include <QHostAddress>
#include <QTimer>
#include <QMutex>
#include <QThread>
#include <QJsonObject>
#include <QJsonDocument>
#include <QStringList>
#include <memory>
#include <atomic>
#include <chrono>
#include <vector>
#include <unordered_map>
#include <functional>
#include <set>

#include "../core/eti_types.hpp"

namespace eti_network {

/**
 * @brief SAP packet header structure (RFC 2974)
 */
struct SapHeader {
    // Version and flags
    uint8_t version : 3;                    // SAP version (1)
    uint8_t address_type : 1;               // IPv4 (0) or IPv6 (1)
    uint8_t reserved : 1;                   // Reserved (0)
    uint8_t deletion_flag : 1;              // Deletion announcement
    uint8_t encryption_flag : 1;            // Encryption bit
    uint8_t compression_flag : 1;           // Compression bit
    
    // Authentication length
    uint8_t auth_length;                    // Authentication data length
    
    // Message identifier hash
    uint16_t message_id_hash;               // Message identifier hash
    
    // Originating source
    uint32_t originating_source;            // IPv4 address of announcer
    
    // Optional authentication data
    std::vector<uint8_t> auth_data;         // Authentication data (if present)
    
    // Payload data
    QByteArray payload_data;                // SDP payload
    
    bool is_valid() const {
        return version == 1 && reserved == 0;
    }
    
    QString to_string() const {
        return QString("SAP v%1 hash=%2 delete=%3 auth_len=%4")
            .arg(version)
            .arg(message_id_hash, 4, 16, QChar('0'))
            .arg(deletion_flag)
            .arg(auth_length);
    }
};

/**
 * @brief SDP session description structure (RFC 4566)
 */
struct SdpSession {
    // Session-level attributes
    QString session_name;                   // Session name (s=)
    QString session_information;            // Session information (i=)
    QString session_uri;                    // Session URI (u=)
    QString session_email;                  // Contact email (e=)
    QString session_phone;                  // Contact phone (p=)
    
    // Connection information
    QString connection_network_type = "IN"; // Network type (c=)
    QString connection_address_type = "IP4"; // Address type
    QHostAddress connection_address;        // Connection address
    uint8_t connection_ttl = 255;          // TTL for multicast
    
    // Timing information
    uint64_t session_start_time = 0;        // Session start (t=)
    uint64_t session_stop_time = 0;         // Session stop
    
    // Bandwidth information
    QString bandwidth_type;                 // Bandwidth modifier (b=)
    uint32_t bandwidth_value = 0;           // Bandwidth value
    
    // Media descriptions
    struct MediaDescription {
        QString media_type;                 // Media type (audio/video/data)
        uint16_t port = 0;                 // Port number
        QString protocol;                   // Protocol (RTP/AVP, UDP, etc.)
        QStringList format_list;           // Format list
        QString media_title;               // Media title (i=)
        QHostAddress media_connection;     // Media connection (c=)
        QStringList attributes;            // Media attributes (a=)
        
        // ETI-specific attributes
        QString eti_format;                // ETI format (ETI-NI, ETI-LI, EDI)
        uint32_t ensemble_id = 0;          // DAB ensemble ID
        QString ensemble_label;            // DAB ensemble label
        QString provider_name;             // Service provider
        double bitrate_kbps = 0.0;         // Stream bitrate
        QString encoding_type;             // Encoding type
        
        bool is_eti_stream() const {
            return media_type == "application" && 
                   (format_list.contains("ETI") || 
                    format_list.contains("eti") ||
                    attributes.contains("eti-format"));
        }
        
        QString to_string() const {
            return QString("%1/%2 %3 %4")
                .arg(media_type)
                .arg(port)
                .arg(protocol)
                .arg(format_list.join(","));
        }
    };
    
    QList<MediaDescription> media_descriptions; // Media descriptions (m=)
    
    // Session attributes
    QStringList session_attributes;         // Session attributes (a=)
    
    // Custom ETI attributes
    QJsonObject custom_attributes;          // Custom attributes
    
    bool is_valid() const {
        return !session_name.isEmpty() && !media_descriptions.isEmpty();
    }
    
    bool contains_eti_streams() const {
        for (const auto& media : media_descriptions) {
            if (media.is_eti_stream()) return true;
        }
        return false;
    }
    
    QList<MediaDescription> get_eti_media() const {
        QList<MediaDescription> eti_media;
        for (const auto& media : media_descriptions) {
            if (media.is_eti_stream()) {
                eti_media.append(media);
            }
        }
        return eti_media;
    }
    
    QString to_sdp_string() const;
    QJsonObject to_json() const;
    void from_json(const QJsonObject& json);
};

/**
 * @brief Discovered broadcast service information
 */
struct DiscoveredBroadcastService {
    // Service identification
    QString service_id;                     // Unique service ID
    QString service_name;                   // Human-readable service name
    QString service_description;            // Service description
    QString provider_name;                  // Service provider
    
    // Network information
    QHostAddress multicast_address;         // Multicast address
    uint16_t port = 0;                     // Port number
    QString protocol;                       // Transport protocol
    QString network_interface;              // Receiving interface
    
    // ETI stream information
    QString eti_format;                     // ETI format type
    uint32_t ensemble_id = 0;              // DAB ensemble ID
    QString ensemble_label;                 // DAB ensemble label
    double bitrate_kbps = 0.0;             // Stream bitrate
    QString encoding_type;                  // Audio encoding
    
    // Quality metrics
    double signal_strength = 0.0;           // Signal strength (0.0-1.0)
    double signal_quality = 1.0;           // Signal quality (0.0-1.0)
    std::chrono::milliseconds last_seen{0}; // Last announcement seen
    size_t announcement_count = 0;          // Total announcements received
    
    // Discovery metadata
    QDateTime first_discovered;             // First discovery time
    QDateTime last_updated;                 // Last update time
    SdpSession session_description;         // Complete SDP session
    QHostAddress announcer_address;         // SAP announcer address
    
    // Service status
    bool is_active = true;                  // Service currently active
    bool is_verified = false;               // Service connectivity verified
    bool is_eti_compatible = false;         // Compatible with ETI analyser
    QString status_message;                 // Status information
    
    // Custom metadata
    QJsonObject custom_metadata;            // Additional metadata
    
    bool is_stale(std::chrono::minutes timeout = std::chrono::minutes{5}) const {
        auto now = std::chrono::steady_clock::now();
        auto last_seen_time = std::chrono::steady_clock::time_point{last_seen};
        return (now - last_seen_time) > timeout;
    }
    
    QString to_string() const {
        return QString("%1 (%2:%3) - %4 [%5]")
            .arg(service_name)
            .arg(multicast_address.toString())
            .arg(port)
            .arg(provider_name)
            .arg(eti_format);
    }
    
    QJsonObject to_json() const;
    void from_json(const QJsonObject& json);
};

/**
 * @brief SAP/SDP discovery configuration
 */
struct DiscoveryConfiguration {
    // SAP configuration
    QHostAddress sap_multicast_address = QHostAddress("224.2.127.254"); // IPv4 SAP address
    uint16_t sap_port = 9875;                                           // SAP port
    QStringList network_interfaces;                                     // Specific interfaces
    bool discover_all_interfaces = true;                                // Auto-discover interfaces
    
    // Discovery behavior
    std::chrono::seconds discovery_timeout{30};                        // Discovery session timeout
    std::chrono::seconds service_timeout{300};                         // Service timeout (5 min)
    std::chrono::milliseconds announcement_interval{1000};            // Re-announcement interval
    bool enable_continuous_discovery = true;                           // Continuous discovery
    
    // Filtering options
    bool filter_eti_only = true;                                       // Only ETI services
    QStringList allowed_protocols;                                      // Allowed protocols
    QStringList blocked_addresses;                                      // Blocked IP addresses
    QStringList required_keywords;                                      // Required keywords
    QStringList excluded_keywords;                                      // Excluded keywords
    
    // Quality requirements
    double min_signal_strength = 0.0;                                  // Minimum signal strength
    double min_signal_quality = 0.8;                                   // Minimum signal quality
    size_t min_announcement_count = 3;                                  // Minimum announcements
    std::chrono::seconds min_service_age{10};                          // Minimum service age
    
    // Advanced features
    bool enable_service_verification = true;                           // Verify service connectivity
    bool enable_metadata_extraction = true;                            // Extract detailed metadata
    bool enable_redundancy_detection = true;                           // Detect redundant services
    bool enable_quality_monitoring = true;                             // Monitor service quality
    
    // Performance tuning
    size_t max_concurrent_discoveries = 10;                            // Max concurrent operations
    size_t discovery_buffer_size = 1000;                               // Discovery buffer size
    bool enable_caching = true;                                        // Enable result caching
    std::chrono::hours cache_expiry{24};                               // Cache expiry time
};

/**
 * @brief Professional SAP packet parser
 */
class SapPacketParser {
public:
    static bool parse_sap_packet(const QByteArray& packet_data, SapHeader& header);
    static bool parse_sdp_payload(const QByteArray& sdp_data, SdpSession& session);
    static bool extract_eti_metadata(const SdpSession& session, DiscoveredBroadcastService& service);
    static QString validate_sap_packet(const QByteArray& packet_data);
    static QString validate_sdp_session(const SdpSession& session);
    
private:
    static bool parse_sdp_line(const QString& line, SdpSession& session);
    static bool parse_media_line(const QString& line, SdpSession::MediaDescription& media);
    static bool parse_connection_line(const QString& line, SdpSession& session);
    static bool parse_attribute_line(const QString& line, SdpSession& session, 
                                   SdpSession::MediaDescription* current_media = nullptr);
    static QHostAddress extract_multicast_address(const SdpSession& session);
    static uint16_t extract_port_number(const SdpSession& session);
    static double extract_bitrate(const SdpSession& session);
};

/**
 * @brief Professional discovery worker thread
 */
class DiscoveryWorkerThread : public QThread {
    Q_OBJECT
    
public:
    explicit DiscoveryWorkerThread(const DiscoveryConfiguration& config, 
                                 const QString& interface_name,
                                 QObject* parent = nullptr);
    ~DiscoveryWorkerThread() override;
    
    // Discovery control
    void start_discovery();
    void stop_discovery();
    bool is_discovering() const;
    
    // Configuration
    void update_config(const DiscoveryConfiguration& config);
    DiscoveryConfiguration get_config() const;
    QString get_interface_name() const;
    
    // Statistics
    size_t get_packets_received() const;
    size_t get_services_discovered() const;
    size_t get_announcements_processed() const;

signals:
    void service_discovered(const DiscoveredBroadcastService& service);
    void service_updated(const DiscoveredBroadcastService& service);
    void service_removed(const QString& service_id);
    void discovery_error(const QString& interface_name, const QString& error, int severity);
    void packet_received(const QString& interface_name, const SapHeader& header);

protected:
    void run() override;

private slots:
    void process_pending_packets();
    void cleanup_stale_services();
    void verify_active_services();

private:
    void initialize_discovery();
    void finalize_discovery();
    void setup_sap_socket();
    void join_sap_multicast();
    void leave_sap_multicast();
    void process_sap_packet(const QByteArray& packet_data, const QHostAddress& sender);
    void update_service_registry(const DiscoveredBroadcastService& service);
    void remove_stale_services();
    bool verify_service_connectivity(const DiscoveredBroadcastService& service);
    bool meets_quality_requirements(const DiscoveredBroadcastService& service);
    
    DiscoveryConfiguration m_config;
    QString m_interface_name;
    std::unique_ptr<QUdpSocket> m_sap_socket;
    std::unique_ptr<QTimer> m_cleanup_timer;
    std::unique_ptr<QTimer> m_verification_timer;
    
    // Service registry
    std::unordered_map<std::string, DiscoveredBroadcastService> m_discovered_services;
    mutable std::mutex m_services_mutex;
    
    // Statistics
    std::atomic<size_t> m_packets_received{0};
    std::atomic<size_t> m_services_discovered{0};
    std::atomic<size_t> m_announcements_processed{0};
    
    std::atomic<bool> m_discovering{false};
    std::atomic<bool> m_initialized{false};
    
    mutable std::mutex m_config_mutex;
    
    static constexpr std::chrono::milliseconds CLEANUP_INTERVAL{60000};      // 1 minute
    static constexpr std::chrono::milliseconds VERIFICATION_INTERVAL{30000}; // 30 seconds
};

/**
 * @brief Production-Grade SAP/SDP Discovery Engine
 * 
 * Professional broadcast network discovery system implementing:
 * - RFC 2974 SAP (Session Announcement Protocol) compliance
 * - RFC 4566 SDP (Session Description Protocol) parsing
 * - Multi-interface discovery with redundancy support
 * - Real-time service directory with metadata extraction
 * - Automatic ETI stream discovery and verification
 * - Professional broadcast workflow integration
 */
class SapSdpDiscoveryEngine : public QObject {
    Q_OBJECT
    
public:
    explicit SapSdpDiscoveryEngine(QObject* parent = nullptr);
    ~SapSdpDiscoveryEngine() override;
    
    // Discovery session management
    bool start_discovery(const DiscoveryConfiguration& config = DiscoveryConfiguration{});
    void stop_discovery();
    bool is_discovery_active() const;
    void restart_discovery();
    
    // Configuration management
    void set_configuration(const DiscoveryConfiguration& config);
    DiscoveryConfiguration get_configuration() const;
    void update_discovery_timeout(std::chrono::seconds timeout);
    void set_quality_requirements(double min_strength, double min_quality);
    
    // Service discovery results
    QList<DiscoveredBroadcastService> get_discovered_services() const;
    QList<DiscoveredBroadcastService> get_eti_services() const;
    DiscoveredBroadcastService get_service(const QString& service_id) const;
    QStringList get_service_ids() const;
    size_t get_service_count() const;
    
    // Service filtering and searching
    QList<DiscoveredBroadcastService> filter_services_by_provider(const QString& provider) const;
    QList<DiscoveredBroadcastService> filter_services_by_protocol(const QString& protocol) const;
    QList<DiscoveredBroadcastService> filter_services_by_quality(double min_quality) const;
    QList<DiscoveredBroadcastService> search_services(const QString& search_term) const;
    
    // Network interface management
    QStringList get_available_interfaces() const;
    QStringList get_active_discovery_interfaces() const;
    void add_discovery_interface(const QString& interface_name);
    void remove_discovery_interface(const QString& interface_name);
    void enable_all_interfaces();
    
    // Service verification and testing
    bool verify_service_connectivity(const QString& service_id);
    bool test_service_streaming(const QString& service_id, std::chrono::milliseconds timeout);
    QString get_service_diagnostic_info(const QString& service_id) const;
    void refresh_service_metadata(const QString& service_id);
    
    // Quality monitoring and metrics
    double get_overall_discovery_health() const;
    QVariantMap get_discovery_statistics() const;
    QVariantMap get_interface_statistics(const QString& interface_name) const;
    void reset_discovery_statistics();
    
    // Advanced features
    void enable_continuous_monitoring(bool enabled);
    void enable_service_caching(bool enabled, std::chrono::hours expiry_time);
    void enable_redundancy_detection(bool enabled);
    void set_discovery_priority(const QStringList& interface_priority);
    
    // Data export and import
    void export_discovered_services(const QString& filename, const QString& format = "json") const;
    void import_service_database(const QString& filename);
    QJsonObject export_discovery_session() const;
    void import_discovery_session(const QJsonObject& session_data);
    
    // Callback registration
    using ServiceCallback = std::function<void(const DiscoveredBroadcastService&)>;
    using ErrorCallback = std::function<void(const QString&, const QString&, int)>;
    using QualityCallback = std::function<void(const QString&, double)>;
    
    void set_service_discovered_callback(ServiceCallback callback);
    void set_service_updated_callback(ServiceCallback callback);
    void set_service_removed_callback(std::function<void(const QString&)> callback);
    void set_error_callback(ErrorCallback callback);
    void set_quality_callback(QualityCallback callback);

signals:
    void discovery_started();
    void discovery_stopped();
    void discovery_error(const QString& error, int severity);
    
    void service_discovered(const DiscoveredBroadcastService& service);
    void service_updated(const DiscoveredBroadcastService& service);
    void service_removed(const QString& service_id);
    void service_verified(const QString& service_id, bool connectivity_ok);
    
    void interface_added(const QString& interface_name);
    void interface_removed(const QString& interface_name);
    void interface_error(const QString& interface_name, const QString& error);
    
    void quality_update(const QString& service_id, double quality_score);
    void discovery_statistics_updated(const QVariantMap& statistics);

private slots:
    void handle_worker_service_discovered(const DiscoveredBroadcastService& service);
    void handle_worker_service_updated(const DiscoveredBroadcastService& service);
    void handle_worker_service_removed(const QString& service_id);
    void handle_worker_error(const QString& interface_name, const QString& error, int severity);
    void perform_global_maintenance();
    void update_discovery_statistics();

private:
    // Worker thread management
    struct DiscoveryInterface {
        QString interface_name;
        std::unique_ptr<DiscoveryWorkerThread> worker;
        bool is_active = false;
        std::chrono::steady_clock::time_point last_activity;
        size_t services_discovered = 0;
        size_t packets_received = 0;
    };
    
    mutable std::mutex m_interfaces_mutex;
    std::unordered_map<std::string, std::unique_ptr<DiscoveryInterface>> m_discovery_interfaces;
    
    // Service registry
    mutable std::mutex m_services_mutex;
    std::unordered_map<std::string, DiscoveredBroadcastService> m_global_service_registry;
    
    // Configuration and state
    DiscoveryConfiguration m_config;
    std::atomic<bool> m_discovery_active{false};
    std::chrono::steady_clock::time_point m_discovery_start_time;
    
    // Maintenance and monitoring
    std::unique_ptr<QTimer> m_maintenance_timer;
    std::unique_ptr<QTimer> m_statistics_timer;
    
    // Caching and optimization
    std::atomic<bool> m_caching_enabled{true};
    std::chrono::hours m_cache_expiry{24};
    
    // Callbacks
    ServiceCallback m_service_discovered_callback;
    ServiceCallback m_service_updated_callback;
    std::function<void(const QString&)> m_service_removed_callback;
    ErrorCallback m_error_callback;
    QualityCallback m_quality_callback;
    mutable std::mutex m_callback_mutex;
    
    // Helper methods
    void initialize_discovery_interfaces();
    void cleanup_discovery_interfaces();
    void create_interface_worker(const QString& interface_name);
    void destroy_interface_worker(const QString& interface_name);
    void merge_service_data(const DiscoveredBroadcastService& new_service);
    void cleanup_stale_services();
    void detect_service_redundancy();
    bool is_interface_suitable_for_discovery(const QString& interface_name) const;
    void validate_discovery_configuration(const DiscoveryConfiguration& config);
    QStringList get_default_discovery_interfaces() const;
    
    static constexpr std::chrono::milliseconds MAINTENANCE_INTERVAL{30000};   // 30 seconds
    static constexpr std::chrono::milliseconds STATISTICS_INTERVAL{5000};    // 5 seconds
    static constexpr size_t MAX_DISCOVERY_INTERFACES = 16;
    static constexpr size_t MAX_CACHED_SERVICES = 1000;
};

/**
 * @brief Factory for creating SAP/SDP discovery engines
 */
class SapSdpDiscoveryEngineFactory {
public:
    /**
     * @brief Create discovery engine for broadcast environments
     */
    static std::unique_ptr<SapSdpDiscoveryEngine> create_broadcast_discovery();
    
    /**
     * @brief Create discovery engine for monitoring applications
     */
    static std::unique_ptr<SapSdpDiscoveryEngine> create_monitoring_discovery();
    
    /**
     * @brief Create discovery engine for development and testing
     */
    static std::unique_ptr<SapSdpDiscoveryEngine> create_development_discovery();
    
    /**
     * @brief Create discovery engine optimized for specific network
     */
    static std::unique_ptr<SapSdpDiscoveryEngine> create_network_optimized_discovery(
        const QStringList& preferred_interfaces);
    
    /**
     * @brief Create custom discovery engine with specific configuration
     */
    static std::unique_ptr<SapSdpDiscoveryEngine> create_custom_discovery(
        const DiscoveryConfiguration& config);
};

} // namespace eti_network

#endif // SAP_SDP_DISCOVERY_ENGINE_HPP