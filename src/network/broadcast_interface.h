/**
 * @file broadcast_interface.h
 * @brief Professional Broadcast Hardware Interface Integration
 * 
 * Implements comprehensive broadcast equipment integration for:
 * - Professional broadcast hardware connectivity (DekTec, Elecard, TeamCast)
 * - Industry standard protocol support (ASI, SPI, ETI-over-IP)
 * - Equipment discovery and management
 * - Redundancy and failover configuration
 * - Real-time monitoring and control
 * - Professional broadcast workflow automation
 * 
 * @author Network/Stream Agent
 * @date 2025-09-21
 * @copyright Copyright (c) 2025 ETI Stream Analyzer Team
 */

#ifndef BROADCAST_INTERFACE_H
#define BROADCAST_INTERFACE_H

#include <QObject>
#include <QTimer>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QTcpSocket>
#include <QUdpSocket>
#include <QJsonObject>
#include <QJsonDocument>
#include <QMutex>
#include <memory>
#include <atomic>
#include <chrono>
#include <functional>
#include <unordered_map>

#include "../core/eti_types.h"
#include "eti_over_ip_receiver.h"

namespace eti_network {

/**
 * @brief Professional broadcast protocol enumeration
 */
enum class BroadcastProtocol {
    ETI_OVER_IP,            // ETI-over-IP multicast (RFC 3550)
    ASI,                    // Asynchronous Serial Interface (DVB-ASI)
    SPI,                    // Serial Parallel Interface
    SDI,                    // Serial Digital Interface
    DTMB,                   // Digital Terrestrial Multimedia Broadcast
    DAB_PLUS,               // DAB+ specific protocol
    EDI,                    // Encapsulated DAB Interface
    HTTP_REST,              // HTTP REST API control
    SNMP,                   // Simple Network Management Protocol
    TELNET,                 // Telnet command interface
    PROPRIETARY             // Vendor-specific protocol
};

/**
 * @brief Timing reference source for synchronization
 */
enum class TimingReference {
    SYSTEM_CLOCK,           // System clock reference
    GPS_PPS,                // GPS Pulse Per Second
    NTP_SYNC,               // Network Time Protocol
    ATOMIC_CLOCK,           // Atomic clock reference
    EXTERNAL_REF,           // External reference signal
    INTERNAL_OSC,           // Internal oscillator
    ETI_SYNC                // ETI stream synchronization
};

/**
 * @brief Equipment type classification
 */
enum class EquipmentType {
    ETI_GENERATOR,          // ETI stream generator
    ETI_ANALYZER,           // ETI stream analyzer
    MODULATOR,              // DAB modulator
    EXCITER,                // RF exciter
    TRANSMITTER,            // DAB transmitter
    GATEWAY,                // Network gateway
    MULTIPLEXER,            // Service multiplexer
    ENCODER,                // Audio encoder
    MONITOR,                // Signal monitor
    TEST_EQUIPMENT,         // Test and measurement
    UNKNOWN                 // Unknown equipment type
};

/**
 * @brief Equipment configuration parameters
 */
struct EquipmentConfig {
    QString equipment_id;                             // Unique equipment identifier
    QString name;                                     // Human-readable name
    QString manufacturer;                             // Equipment manufacturer
    QString model;                                    // Equipment model
    QString firmware_version;                         // Firmware version
    QString serial_number;                            // Serial number
    
    // Network configuration
    QString ip_address;                               // Equipment IP address
    quint16 control_port = 0;                        // Control port
    quint16 data_port = 0;                           // Data port
    QString username;                                 // Authentication username
    QString password;                                 // Authentication password
    
    // Protocol configuration
    BroadcastProtocol primary_protocol;              // Primary protocol
    QList<BroadcastProtocol> supported_protocols;    // Supported protocols
    TimingReference timing_reference;                // Timing reference
    
    // Capabilities
    EquipmentType equipment_type;                     // Equipment type
    QStringList supported_formats;                    // Supported stream formats
    QStringList supported_standards;                  // Supported broadcast standards
    bool supports_redundancy = false;                // Redundancy support
    bool supports_remote_control = false;            // Remote control support
    bool supports_monitoring = false;                // Monitoring support
    
    // Custom parameters
    QJsonObject custom_parameters;                    // Vendor-specific parameters
    
    bool is_valid() const {
        return !equipment_id.isEmpty() && 
               !ip_address.isEmpty() && 
               control_port > 0;
    }
};

/**
 * @brief Broadcast parameters for equipment configuration
 */
struct BroadcastParams {
    // ETI stream parameters
    QString ensemble_name;                            // Ensemble name
    quint16 ensemble_id = 0;                         // Ensemble identifier
    QString country_code;                            // Country code
    quint8 extended_country_code = 0;                // Extended country code
    
    // DAB parameters
    quint8 transmission_mode = 1;                    // DAB transmission mode (1-4)
    double center_frequency_khz = 0.0;               // Center frequency in kHz
    double output_power_dbm = 0.0;                   // Output power in dBm
    double guard_interval_ratio = 0.25;              // Guard interval ratio
    
    // Service configuration
    QJsonArray services;                             // Service configuration array
    size_t total_capacity_cu = 864;                  // Total capacity in CU
    size_t used_capacity_cu = 0;                     // Used capacity in CU
    
    // Quality parameters
    double signal_quality_threshold = 0.8;           // Signal quality threshold
    double ber_threshold = 1e-4;                     // Bit error rate threshold
    double snr_threshold_db = 20.0;                  // SNR threshold in dB
    
    // Timing parameters
    TimingReference timing_source;                    // Timing source
    std::chrono::microseconds sync_tolerance{100};   // Sync tolerance
    bool enable_time_interleaving = true;            // Time interleaving
    
    // Advanced parameters
    bool enable_fic_redundancy = true;               // FIC redundancy
    bool enable_eep_protection = true;               // Equal Error Protection
    quint8 subchannel_organization = 0;              // Subchannel organization
    
    QJsonObject to_json() const {
        QJsonObject obj;
        obj["ensemble_name"] = ensemble_name;
        obj["ensemble_id"] = static_cast<int>(ensemble_id);
        obj["country_code"] = country_code;
        obj["extended_country_code"] = static_cast<int>(extended_country_code);
        obj["transmission_mode"] = static_cast<int>(transmission_mode);
        obj["center_frequency_khz"] = center_frequency_khz;
        obj["output_power_dbm"] = output_power_dbm;
        obj["guard_interval_ratio"] = guard_interval_ratio;
        obj["services"] = services;
        obj["total_capacity_cu"] = static_cast<int>(total_capacity_cu);
        obj["used_capacity_cu"] = static_cast<int>(used_capacity_cu);
        return obj;
    }
};

/**
 * @brief Equipment manager for device discovery and management
 */
class EquipmentManager : public QObject {
    Q_OBJECT
    
public:
    explicit EquipmentManager(QObject* parent = nullptr);
    ~EquipmentManager();
    
    // Equipment discovery
    void startDiscovery();
    void stopDiscovery();
    void scanNetworkRange(const QString& networkRange);
    void addManualEquipment(const EquipmentConfig& config);
    void removeEquipment(const QString& equipmentId);
    
    // Equipment access
    QStringList getDiscoveredEquipment() const;
    EquipmentConfig getEquipmentConfig(const QString& equipmentId) const;
    QList<EquipmentConfig> getEquipmentByType(EquipmentType type) const;
    QList<EquipmentConfig> getEquipmentByManufacturer(const QString& manufacturer) const;
    
    // Equipment status
    bool isEquipmentOnline(const QString& equipmentId) const;
    QString getEquipmentStatus(const QString& equipmentId) const;
    QDateTime getLastSeen(const QString& equipmentId) const;
    
    // Configuration management
    bool updateEquipmentConfig(const QString& equipmentId, const EquipmentConfig& config);
    bool validateEquipmentConfig(const EquipmentConfig& config) const;
    void setDiscoveryInterval(std::chrono::milliseconds interval);
    
signals:
    void equipmentDiscovered(const EquipmentConfig& config);
    void equipmentRemoved(const QString& equipmentId);
    void equipmentStatusChanged(const QString& equipmentId, const QString& status);
    void equipmentConfigUpdated(const QString& equipmentId, const EquipmentConfig& config);
    
private slots:
    void performDiscovery();
    void checkEquipmentStatus();
    
private:
    void discoverByProtocol(BroadcastProtocol protocol);
    void probeEquipment(const QString& ipAddress, quint16 port);
    EquipmentConfig identifyEquipment(const QString& ipAddress, quint16 port);
    void updateEquipmentStatus(const QString& equipmentId, const QString& status);
    
    std::unique_ptr<QTimer> m_discoveryTimer;
    std::unique_ptr<QTimer> m_statusTimer;
    std::unique_ptr<QNetworkAccessManager> m_networkManager;
    
    mutable QMutex m_equipmentMutex;
    std::unordered_map<QString, EquipmentConfig> m_equipment;
    std::unordered_map<QString, QString> m_equipmentStatus;
    std::unordered_map<QString, QDateTime> m_lastSeen;
    
    std::chrono::milliseconds m_discoveryInterval{30000};
    std::atomic<bool> m_discoveryActive{false};
    
    static constexpr std::chrono::milliseconds STATUS_CHECK_INTERVAL{10000};
    static constexpr std::chrono::seconds EQUIPMENT_TIMEOUT{60};
};

/**
 * @brief Protocol handler for equipment communication
 */
class ProtocolHandler : public QObject {
    Q_OBJECT
    
public:
    explicit ProtocolHandler(QObject* parent = nullptr);
    ~ProtocolHandler();
    
    // Protocol management
    bool addProtocolSupport(BroadcastProtocol protocol);
    void removeProtocolSupport(BroadcastProtocol protocol);
    QList<BroadcastProtocol> getSupportedProtocols() const;
    bool isProtocolSupported(BroadcastProtocol protocol) const;
    
    // Equipment communication
    bool connectToEquipment(const QString& equipmentId, const EquipmentConfig& config);
    void disconnectFromEquipment(const QString& equipmentId);
    bool isConnected(const QString& equipmentId) const;
    
    // Command execution
    QJsonObject sendCommand(const QString& equipmentId, const QString& command, 
                           const QJsonObject& parameters = QJsonObject());
    QJsonObject getStatus(const QString& equipmentId);
    bool setParameters(const QString& equipmentId, const BroadcastParams& params);
    BroadcastParams getParameters(const QString& equipmentId);
    
    // Stream control
    bool startStream(const QString& equipmentId, const QJsonObject& streamConfig);
    bool stopStream(const QString& equipmentId);
    bool configureStream(const QString& equipmentId, const QJsonObject& streamConfig);
    
    // Monitoring
    void enableMonitoring(const QString& equipmentId, bool enabled);
    QJsonObject getMonitoringData(const QString& equipmentId);
    void setMonitoringInterval(std::chrono::milliseconds interval);
    
signals:
    void equipmentConnected(const QString& equipmentId);
    void equipmentDisconnected(const QString& equipmentId);
    void commandResponse(const QString& equipmentId, const QString& command, const QJsonObject& response);
    void monitoringData(const QString& equipmentId, const QJsonObject& data);
    void protocolError(const QString& equipmentId, const QString& error);
    
private slots:
    void handleNetworkReply();
    void handleTcpConnected();
    void handleTcpDisconnected();
    void handleTcpError(QAbstractSocket::SocketError error);
    void performMonitoring();
    
private:
    // Protocol implementations
    QJsonObject executeHttpCommand(const EquipmentConfig& config, const QString& command, 
                                  const QJsonObject& parameters);
    QJsonObject executeTelnetCommand(const EquipmentConfig& config, const QString& command, 
                                    const QJsonObject& parameters);
    QJsonObject executeSnmpCommand(const EquipmentConfig& config, const QString& command, 
                                  const QJsonObject& parameters);
    
    void setupProtocolConnections(const QString& equipmentId, const EquipmentConfig& config);
    void cleanupProtocolConnections(const QString& equipmentId);
    
    std::unique_ptr<QNetworkAccessManager> m_networkManager;
    std::unique_ptr<QTimer> m_monitoringTimer;
    
    mutable QMutex m_connectionsMutex;
    std::unordered_map<QString, EquipmentConfig> m_connections;
    std::unordered_map<QString, std::unique_ptr<QTcpSocket>> m_tcpSockets;
    std::unordered_map<QString, QNetworkReply*> m_activeRequests;
    
    QList<BroadcastProtocol> m_supportedProtocols;
    std::chrono::milliseconds m_monitoringInterval{5000};
    std::atomic<bool> m_monitoringEnabled{false};
    
    static constexpr std::chrono::milliseconds COMMAND_TIMEOUT{10000};
    static constexpr std::chrono::milliseconds TCP_CONNECT_TIMEOUT{5000};
};

/**
 * @brief Redundancy controller for equipment failover management
 */
class RedundancyController : public QObject {
    Q_OBJECT
    
public:
    explicit RedundancyController(QObject* parent = nullptr);
    ~RedundancyController();
    
    // Redundancy configuration
    void addRedundancyGroup(const QString& groupId, const QStringList& equipmentIds);
    void removeRedundancyGroup(const QString& groupId);
    void setActiveEquipment(const QString& groupId, const QString& equipmentId);
    QString getActiveEquipment(const QString& groupId) const;
    
    // Failover management
    void enableAutomaticFailover(const QString& groupId, bool enabled);
    void setFailoverThreshold(const QString& groupId, double threshold);
    void setFailoverDelay(const QString& groupId, std::chrono::milliseconds delay);
    void triggerManualFailover(const QString& groupId, const QString& targetEquipmentId);
    
    // Status monitoring
    QStringList getRedundancyGroups() const;
    QStringList getGroupEquipment(const QString& groupId) const;
    bool isFailoverActive(const QString& groupId) const;
    QString getFailoverReason(const QString& groupId) const;
    QDateTime getLastFailoverTime(const QString& groupId) const;
    
    // Health monitoring
    void updateEquipmentHealth(const QString& equipmentId, double healthScore);
    double getEquipmentHealth(const QString& equipmentId) const;
    void setHealthCheckInterval(std::chrono::milliseconds interval);
    
signals:
    void redundancyGroupAdded(const QString& groupId);
    void redundancyGroupRemoved(const QString& groupId);
    void activeEquipmentChanged(const QString& groupId, const QString& equipmentId);
    void failoverTriggered(const QString& groupId, const QString& fromEquipment, const QString& toEquipment);
    void failoverCompleted(const QString& groupId, bool success);
    void equipmentHealthChanged(const QString& equipmentId, double healthScore);
    
private slots:
    void checkRedundancyStatus();
    void performHealthCheck();
    
private:
    struct RedundancyGroup {
        QString group_id;
        QStringList equipment_ids;
        QString active_equipment;
        bool automatic_failover = true;
        double failover_threshold = 0.7;
        std::chrono::milliseconds failover_delay{5000};
        bool failover_active = false;
        QString failover_reason;
        QDateTime last_failover;
    };
    
    void evaluateFailoverConditions(const QString& groupId);
    void executeFailover(const QString& groupId, const QString& targetEquipmentId, const QString& reason);
    bool validateFailoverTarget(const QString& groupId, const QString& equipmentId);
    
    std::unique_ptr<QTimer> m_statusTimer;
    std::unique_ptr<QTimer> m_healthTimer;
    
    mutable QMutex m_redundancyMutex;
    std::unordered_map<QString, RedundancyGroup> m_redundancyGroups;
    std::unordered_map<QString, double> m_equipmentHealth;
    
    std::chrono::milliseconds m_healthCheckInterval{5000};
    
    static constexpr std::chrono::milliseconds STATUS_CHECK_INTERVAL{2000};
    static constexpr double DEFAULT_HEALTH_SCORE = 1.0;
    static constexpr double CRITICAL_HEALTH_THRESHOLD = 0.5;
};

/**
 * @brief Synchronization engine for timing and phase alignment
 */
class SynchronizationEngine : public QObject {
    Q_OBJECT
    
public:
    explicit SynchronizationEngine(QObject* parent = nullptr);
    ~SynchronizationEngine();
    
    // Timing reference management
    void setTimingReference(TimingReference reference);
    TimingReference getTimingReference() const;
    void calibrateTimingReference();
    bool isTimingReferenceLocked() const;
    
    // Synchronization control
    void enableSynchronization(bool enabled);
    bool isSynchronizationEnabled() const;
    void setSynchronizationTolerance(std::chrono::microseconds tolerance);
    std::chrono::microseconds getSynchronizationTolerance() const;
    
    // Phase alignment
    void alignEquipmentPhase(const QString& equipmentId);
    void setPhaseOffset(const QString& equipmentId, std::chrono::microseconds offset);
    std::chrono::microseconds getPhaseOffset(const QString& equipmentId) const;
    
    // Time synchronization
    QDateTime getCurrentReferenceTime() const;
    std::chrono::microseconds getTimingAccuracy() const;
    void synchronizeSystemClock();
    
    // Network time synchronization
    void configureNtpServer(const QString& ntpServer);
    void enableNtpSynchronization(bool enabled);
    bool isNtpSynchronized() const;
    
signals:
    void timingReferenceLocked();
    void timingReferenceLost();
    void synchronizationStatusChanged(bool synchronized);
    void phaseAlignmentCompleted(const QString& equipmentId);
    void timingAccuracyChanged(std::chrono::microseconds accuracy);
    
private slots:
    void updateTimingStatus();
    void checkSynchronization();
    void performNtpSync();
    
private:
    void initializeTimingReference();
    void monitorTimingStability();
    void adjustPhaseAlignment();
    void calculateTimingAccuracy();
    
    std::unique_ptr<QTimer> m_timingTimer;
    std::unique_ptr<QTimer> m_syncTimer;
    std::unique_ptr<QTimer> m_ntpTimer;
    
    TimingReference m_timingReference = TimingReference::SYSTEM_CLOCK;
    std::atomic<bool> m_timingLocked{false};
    std::atomic<bool> m_synchronizationEnabled{false};
    std::chrono::microseconds m_syncTolerance{100};
    
    mutable QMutex m_phaseMutex;
    std::unordered_map<QString, std::chrono::microseconds> m_phaseOffsets;
    
    // NTP synchronization
    QString m_ntpServer;
    std::atomic<bool> m_ntpEnabled{false};
    std::atomic<bool> m_ntpSynchronized{false};
    
    // Timing accuracy tracking
    std::atomic<int64_t> m_timingAccuracy{1000}; // microseconds
    std::chrono::steady_clock::time_point m_lastCalibration;
    
    static constexpr std::chrono::milliseconds TIMING_UPDATE_INTERVAL{1000};
    static constexpr std::chrono::milliseconds SYNC_CHECK_INTERVAL{5000};
    static constexpr std::chrono::milliseconds NTP_SYNC_INTERVAL{300000}; // 5 minutes
};

/**
 * @brief Professional Broadcast Interface Integration
 * 
 * Comprehensive broadcast equipment interface implementing:
 * - Multi-protocol equipment connectivity and control
 * - Automatic equipment discovery and management
 * - Redundancy and failover configuration
 * - Professional timing and synchronization
 * - Real-time monitoring and alerting
 * - Broadcast workflow automation
 */
class BroadcastInterface : public QObject {
    Q_OBJECT
    
public:
    explicit BroadcastInterface(QObject* parent = nullptr);
    ~BroadcastInterface();
    
    // Equipment management
    bool connectToBroadcastEquipment(const EquipmentConfig& config);
    void disconnectFromEquipment(const QString& equipmentId);
    QStringList getConnectedEquipment() const;
    EquipmentConfig getEquipmentConfig(const QString& equipmentId) const;
    
    // Broadcast configuration
    void configureBroadcastParameters(const BroadcastParams& params);
    BroadcastParams getBroadcastParameters() const;
    void updateEquipmentParameters(const QString& equipmentId, const BroadcastParams& params);
    
    // Redundancy management
    void enableRedundancy(bool enabled);
    bool isRedundancyEnabled() const;
    void configureRedundancyGroup(const QString& groupId, const QStringList& equipmentIds);
    void setActiveEquipment(const QString& groupId, const QString& equipmentId);
    
    // Protocol configuration
    void setProtocolMode(BroadcastProtocol protocol);
    BroadcastProtocol getProtocolMode() const;
    QList<BroadcastProtocol> getSupportedProtocols() const;
    
    // Timing and synchronization
    void configureTimingReference(TimingReference reference);
    TimingReference getTimingReference() const;
    void enableSynchronization(bool enabled);
    bool isSynchronized() const;
    
    // Stream control
    bool startBroadcast(const QString& equipmentId);
    bool stopBroadcast(const QString& equipmentId);
    bool configureBroadcastStream(const QString& equipmentId, const QJsonObject& streamConfig);
    
    // Monitoring and status
    QJsonObject getEquipmentStatus(const QString& equipmentId) const;
    QJsonObject getSystemStatus() const;
    void enableEquipmentMonitoring(const QString& equipmentId, bool enabled);
    void setMonitoringInterval(std::chrono::milliseconds interval);
    
    // Discovery and automation
    void startEquipmentDiscovery();
    void stopEquipmentDiscovery();
    void enableWorkflowAutomation(bool enabled);
    void defineWorkflow(const QString& workflowId, const QJsonObject& workflow);
    
    // Diagnostics and maintenance
    QString getDiagnosticReport() const;
    QStringList getEquipmentAlerts() const;
    void performEquipmentMaintenance(const QString& equipmentId);
    void exportConfiguration(const QString& filename) const;
    bool importConfiguration(const QString& filename);

signals:
    void equipmentConnected(const QString& equipmentId);
    void equipmentDisconnected(const QString& equipmentId);
    void broadcastParametersChanged(const BroadcastParams& params);
    void redundancyStatusChanged(bool active);
    void synchronizationStatusChanged(bool synchronized);
    void equipmentAlert(const QString& equipmentId, const QString& alert);
    void broadcastStarted(const QString& equipmentId);
    void broadcastStopped(const QString& equipmentId);
    void workflowCompleted(const QString& workflowId, bool success);
    void discoveryCompleted(int equipmentCount);

private slots:
    void handleEquipmentDiscovered(const EquipmentConfig& config);
    void handleEquipmentStatusChanged(const QString& equipmentId, const QString& status);
    void handleRedundancyFailover(const QString& groupId, const QString& fromEquipment, const QString& toEquipment);
    void handleSynchronizationStatus(bool synchronized);
    void handleMonitoringData(const QString& equipmentId, const QJsonObject& data);
    void processWorkflowTasks();

private:
    // Component initialization
    void initializeBroadcastInterface();
    void cleanupBroadcastInterface();
    void setupComponentConnections();
    
    // Equipment lifecycle management
    void registerEquipment(const EquipmentConfig& config);
    void unregisterEquipment(const QString& equipmentId);
    void updateEquipmentStatus(const QString& equipmentId, const QString& status);
    
    // Workflow automation
    void executeWorkflow(const QString& workflowId);
    void validateWorkflow(const QJsonObject& workflow);
    bool executeWorkflowStep(const QJsonObject& step);
    
    // Alert management
    void processEquipmentAlert(const QString& equipmentId, const QString& alert);
    void updateAlertStatus(const QString& equipmentId, const QString& alert, bool resolved);
    
    // Core components
    std::unique_ptr<EquipmentManager> m_equipmentManager;
    std::unique_ptr<ProtocolHandler> m_protocolHandler;
    std::unique_ptr<RedundancyController> m_redundancyController;
    std::unique_ptr<SynchronizationEngine> m_syncEngine;
    
    // Workflow management
    std::unique_ptr<QTimer> m_workflowTimer;
    QJsonObject m_workflows;
    bool m_workflowAutomation = false;
    
    // Configuration and state
    BroadcastParams m_broadcastParams;
    BroadcastProtocol m_protocolMode = BroadcastProtocol::ETI_OVER_IP;
    bool m_redundancyEnabled = false;
    
    // Equipment tracking
    mutable QMutex m_equipmentMutex;
    std::unordered_map<QString, EquipmentConfig> m_connectedEquipment;
    std::unordered_map<QString, QJsonObject> m_equipmentStatus;
    std::unordered_map<QString, QStringList> m_equipmentAlerts;
    
    // Performance monitoring
    std::chrono::steady_clock::time_point m_interfaceStartTime;
    QStringList m_performanceLog;
    
    static constexpr std::chrono::milliseconds WORKFLOW_PROCESSING_INTERVAL{1000};
    static constexpr std::chrono::milliseconds DEFAULT_MONITORING_INTERVAL{5000};
    static constexpr size_t MAX_PERFORMANCE_LOG_SIZE = 1000;
};

} // namespace eti_network

#endif // BROADCAST_INTERFACE_H