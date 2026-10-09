// src/core/shared_eti_data.hpp
#ifndef SHARED_ETI_DATA_HPP
#define SHARED_ETI_DATA_HPP

#include <QObject>
#include <QMutex>
#include <QSharedPointer>
#include <QByteArray>
#include <QMap>
#include <QDateTime>
#include <QString>
#include <QList>
#include <cstdint>

namespace streamdab {
namespace core {

/**
 * @brief ETI Frame data structure
 */
struct ETIFrame {
    uint8_t err;           // Error field
    uint8_t fct;           // Frame Count
    uint8_t nst;           // Number of streams
    uint8_t mid;           // Mode identity
    uint8_t fp;            // Frame phase
    uint64_t frame_number; // Sequential frame number
    QDateTime timestamp;   // Frame timestamp
    QByteArray raw_data;   // Full 6144-byte frame
    bool crc_valid;        // CRC validation result
    
    ETIFrame() : err(0), fct(0), nst(0), mid(0), fp(0), 
                 frame_number(0), crc_valid(false) {}
};

/**
 * @brief FIG information structure
 */
struct FIGInfo {
    uint8_t fig_type;      // FIG type (0, 1, 2, etc.)
    uint8_t fig_extension; // Extension number
    QString description;   // Human-readable description
    QByteArray raw_data;   // Raw FIG data
    
    FIGInfo() : fig_type(0), fig_extension(0) {}
};

/**
 * @brief FIC (Fast Information Channel) data
 */
struct FICData {
    QByteArray fic_content;        // Raw 96-byte FIC content
    QList<FIGInfo> figs;           // Parsed FIG structures
    uint16_t ensemble_id;          // Ensemble identifier
    QString ensemble_label;        // Ensemble label
    QDateTime last_update;         // Last update timestamp
    
    FICData() : ensemble_id(0) {}
};

/**
 * @brief Service information
 */
struct ServiceInfo {
    uint32_t service_id;           // Service identifier
    QString service_label;         // Service label (UTF-8)
    uint8_t subchannel_id;         // Sub-channel ID
    bool is_audio;                 // Audio service flag
    bool is_data;                  // Data service flag
    bool is_dabplus;               // DAB+ flag
    QString programme_type;        // Programme type description
    QString language;              // Language code
    int bitrate;                   // Bitrate in kbps
    
    ServiceInfo() : service_id(0), subchannel_id(0), is_audio(false),
                    is_data(false), is_dabplus(false), bitrate(0) {}
};

/**
 * @brief Sub-channel configuration
 */
struct SubchannelConfig {
    uint8_t subchannel_id;         // Sub-channel ID
    uint16_t start_address;        // Start address in CUs
    uint16_t size;                 // Size in CUs
    uint8_t protection_level;      // Protection level (UEP/EEP)
    bool is_uep;                   // UEP mode flag
    
    SubchannelConfig() : subchannel_id(0), start_address(0), 
                        size(0), protection_level(0), is_uep(false) {}
};

/**
 * @brief MSC (Main Service Channel) data
 */
struct MSCData {
    uint8_t subchannel_id;         // Sub-channel ID
    QByteArray stream_data;        // Stream data
    bool is_dabplus;               // DAB+ flag
    bool is_packet_mode;           // Packet mode flag
    int data_rate;                 // Data rate in kbps
    
    MSCData() : subchannel_id(0), is_dabplus(false), 
                is_packet_mode(false), data_rate(0) {}
};

/**
 * @brief Error statistics
 */
struct ErrorStats {
    uint64_t total_frames;         // Total frames processed
    uint64_t crc_errors;           // CRC error count
    uint64_t sync_errors;          // Sync error count
    uint64_t fic_errors;           // FIC error count
    double avg_processing_time_ms; // Average processing time
    double max_processing_time_ms; // Maximum processing time
    QDateTime last_reset;          // Last reset timestamp
    
    ErrorStats() : total_frames(0), crc_errors(0), sync_errors(0),
                   fic_errors(0), avg_processing_time_ms(0.0),
                   max_processing_time_ms(0.0) {}
};

/**
 * @brief Pattern 5: Shared Data Model with Mutex
 * 
 * Thread-safe shared data container for ETI processing.
 * - QMutex protection for all shared data
 * - QSharedPointer for zero-copy MSC data
 * - Signals emitted AFTER mutex unlock (deadlock prevention)
 */
class SharedETIData : public QObject {
    Q_OBJECT

public:
    explicit SharedETIData(QObject *parent = nullptr);
    ~SharedETIData() override = default;

    // Thread-safe setters (emit signals AFTER mutex unlock)
    void setCurrentFrame(const ETIFrame& frame);
    void setFICData(const FICData& fic);
    void updateService(const ServiceInfo& service);
    void setSubchannelConfig(uint8_t subchannel_id, const SubchannelConfig& config);
    void setMSCData(uint8_t subchannel_id, QSharedPointer<QByteArray> data);
    void updateErrorStats(const ErrorStats& stats);
    void incrementErrorCount(const QString& error_type);
    
    // Thread-safe getters (return copies with mutex lock)
    ETIFrame getCurrentFrame() const;
    FICData getFICData() const;
    QMap<uint32_t, ServiceInfo> getServices() const;
    ServiceInfo getService(uint32_t service_id) const;
    QMap<uint8_t, SubchannelConfig> getSubchannels() const;
    QSharedPointer<QByteArray> getMSCData(uint8_t subchannel_id) const;
    ErrorStats getErrorStats() const;
    
    // Utility methods
    void clearAll();
    void clearServices();
    void resetErrorStats();
    QDateTime getLastUpdateTime() const;
    int getServiceCount() const;
    int getSubchannelCount() const;

signals:
    // Specialized signals per Pattern 7
    void frameUpdated(uint64_t frame_number);
    void ficDataUpdated();
    void serviceListUpdated(int service_count);
    void serviceAdded(uint32_t service_id, QString service_label);
    void mscDataUpdated(uint8_t subchannel_id);
    void errorStatsUpdated();
    void ensembleInfoChanged(uint16_t ensemble_id, QString ensemble_label);

private:
    mutable QMutex m_mutex;  // RAII-pattern mutex
    
    // Shared data members
    ETIFrame m_current_frame;
    FICData m_fic_data;
    QMap<uint32_t, ServiceInfo> m_services;
    QMap<uint8_t, SubchannelConfig> m_subchannels;
    QMap<uint8_t, QSharedPointer<QByteArray>> m_msc_data;
    ErrorStats m_error_stats;
    QDateTime m_last_update;
};

} // namespace core
} // namespace streamdab

// Qt metatype declarations
Q_DECLARE_METATYPE(streamdab::core::ETIFrame)
Q_DECLARE_METATYPE(streamdab::core::FICData)
Q_DECLARE_METATYPE(streamdab::core::ServiceInfo)
Q_DECLARE_METATYPE(streamdab::core::SubchannelConfig)
Q_DECLARE_METATYPE(streamdab::core::MSCData)
Q_DECLARE_METATYPE(streamdab::core::ErrorStats)

#endif // SHARED_ETI_DATA_HPP
