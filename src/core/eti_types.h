#ifndef ETI_TYPES_H
#define ETI_TYPES_H

#include <QtCore/QByteArray>
#include <QtCore/QString>
#include <QtCore/QList>
#include <QtCore/QDateTime>
#include <cstdint>

/**
 * @brief Data structures for ETI processing and DAB Analyser interface
 * 
 * These structures define the core data types used throughout the
 * StreamDAB Stream Analyser for ETI frame processing, service discovery,
 * and ensemble management.
 */

/**
 * @brief ETI Frame structure
 * 
 * Represents a single 6144-byte ETI frame with decoded content
 * Supports both ETI-NI and ETI-LI formats including Bangkok patterns
 */
struct ETIFrame {
    QByteArray data;           // Raw 6144-byte frame data
    QString format;            // Frame format (ETI-NI, ETI-LI-A, ETI-LI-B)
    bool syncValid;            // SYNC pattern validation status
    bool crcValid;             // CRC validation status
    QString mscContent;        // Decoded Main Service Channel content
    QString ficContent;        // Fast Information Channel content
    uint32_t frameNumber;      // Sequential frame number
    uint32_t timestamp;        // Timestamp in milliseconds (24ms intervals)
    
    ETIFrame() 
        : syncValid(false)
        , crcValid(false)
        , frameNumber(0)
        , timestamp(0)
    {}
};

/**
 * @brief DAB Service Information
 * 
 * Contains complete information about a discovered DAB service
 * Used for service tree population and detail display
 * 
 * NAMING CONVENTION (Priority 3):
 * - Canonical: camelCase (serviceId, ensembleId, subchannelId)
 * - Use these names consistently in all new code
 * - Do NOT use: service_id, ServiceId, service-id variants
 */
struct ServiceInfo {
    uint32_t serviceId;        // Service Identifier (SID)
    uint32_t ensembleId;       // Ensemble Identifier (EID)
    uint8_t subchannelId;      // Subchannel ID
    QString label;             // Service label (UTF-8)
    QString description;       // Service description
    QString type;              // Service type (Audio, Data, DAB+, etc.)
    uint16_t bitrate;          // Bitrate in kbps
    QString protection;        // Protection level (UEP/EEP)
    bool isAudio;              // True for audio services
    bool isData;               // True for data services
    
    ServiceInfo()
        : serviceId(0)
        , ensembleId(0)
        , subchannelId(0)
        , bitrate(0)
        , isAudio(false)
        , isData(false)
    {}
};

/**
 * @brief DAB Ensemble Information
 * 
 * Contains ensemble-level information for DAB multiplex
 * Used for ensemble tree root and service organization
 */
struct EnsembleInfo {
    uint32_t ensembleId;       // Ensemble Identifier (EID)
    QString ensembleLabel;     // Ensemble label (full)
    QString shortLabel;        // Short label (8 chars max)
    QString countryCode;       // Country code string (e.g., "TH" for Thailand)
    uint8_t countryId;         // Country ID (numeric)
    uint8_t extendedCountryCode; // Extended country code
    uint8_t cifCount;          // Common Interleaved Frame count
    QList<ServiceInfo> services; // List of services in ensemble
    uint16_t serviceCount;     // Total number of services
    QString transmitterInfo;   // Transmitter identification
    QDateTime lastUpdated;     // Last update timestamp
    
    // Backward compatibility alias
    QString label;             // Alias for ensembleLabel (for compatibility)
    
    EnsembleInfo()
        : ensembleId(0)
        , countryId(0)
        , extendedCountryCode(0)
        , cifCount(0)
        , serviceCount(0)
    {
        label = ensembleLabel; // Keep alias in sync
    }
};

/**
 * @brief FIG (Fast Information Group) Data
 * 
 * Contains decoded FIG information for FIC-XPlorer display
 * Supports all major FIG types for comprehensive analysis
 */
struct FIGData {
    uint8_t figType;           // FIG type (0, 1, etc.)
    uint8_t figExtension;      // FIG extension (0/0, 0/1, 1/0, etc.)
    QByteArray rawData;        // Raw FIG data
    QString decodedContent;    // Human-readable decoded content
    bool isValid;              // Validation status
    QString description;       // FIG type description
    
    FIGData()
        : figType(0)
        , figExtension(0)
        , isValid(false)
    {}
};

/**
 * @brief Signal Quality Metrics
 * 
 * Contains real-time signal quality information
 * Used for signal quality panel display
 */
struct SignalQuality {
    double signalStrength;     // Signal strength in dBm
    double snr;                // Signal-to-Noise Ratio in dB
    double frequencyOffset;    // Frequency offset in Hz
    uint8_t qualityPercent;    // Overall quality percentage (0-100)
    uint32_t errorRate;        // Error rate (errors per million)
    bool isLocked;             // Signal lock status
    
    SignalQuality()
        : signalStrength(-999.0)
        , snr(0.0)
        , frequencyOffset(0.0)
        , qualityPercent(0)
        , errorRate(0)
        , isLocked(false)
    {}
};

/**
 * @brief Processing Performance Metrics
 * 
 * Contains real-time processing performance information
 * Used for system status display
 */
struct PerformanceMetrics {
    double processingFPS;      // Frames per second processing rate
    double memoryUsageMB;      // Memory usage in megabytes
    double cpuUsagePercent;    // CPU usage percentage
    uint32_t totalFrames;      // Total frames processed
    uint32_t errorFrames;      // Number of error frames
    bool isRealTime;           // Real-time processing mode
    
    PerformanceMetrics()
        : processingFPS(0.0)
        , memoryUsageMB(0.0)
        , cpuUsagePercent(0.0)
        , totalFrames(0)
        , errorFrames(0)
        , isRealTime(false)
    {}
};

/**
 * @brief ETSI Compliance Error
 * 
 * Contains information about ETSI standard compliance violations
 * Used for error reporting and system messages
 */
struct ETSIComplianceError {
    QString standard;          // ETSI standard reference (e.g., "EN 300 799")
    QString errorCode;         // Specific error code
    QString description;       // Human-readable error description
    uint32_t frameNumber;      // Frame number where error occurred
    QString severity;          // Error severity (Critical, Warning, Info)
    QString suggestion;        // Suggested fix or explanation
    
    ETSIComplianceError()
        : frameNumber(0)
    {}
};

/**
 * @brief Network Stream Status
 * 
 * Contains information about network streaming status
 * Used for real-time ETI-over-IP monitoring
 */
struct NetworkStreamStatus {
    bool isConnected;          // Connection status
    QString remoteAddress;     // Remote IP address
    uint16_t remotePort;       // Remote port number
    double dataRate;           // Data rate in Mbps
    uint32_t packetsReceived;  // Total packets received
    uint32_t packetsLost;      // Lost packets count
    QString connectionType;    // Connection type (UDP, TCP, etc.)
    
    NetworkStreamStatus()
        : isConnected(false)
        , remotePort(0)
        , dataRate(0.0)
        , packetsReceived(0)
        , packetsLost(0)
    {}
};

// Type aliases for convenience
using ETIFrameList = QList<ETIFrame>;
using ServiceInfoList = QList<ServiceInfo>;
using FIGDataList = QList<FIGData>;

#endif // ETI_TYPES_H