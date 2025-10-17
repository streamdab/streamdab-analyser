#ifndef ADVANCED_FIG_ANALYZER_H
#define ADVANCED_FIG_ANALYZER_H

#include <QtCore/QObject>
#include <QtCore/QByteArray>
#include <QtCore/QString>
#include <QtCore/QList>
#include <QtCore/QMap>
#include <QtCore/QDateTime>
#include <cstdint>
#include <array>
#include <vector>
#include <map>
#include "eti_types.h"

/**
 * DAB Label Structure per ETSI EN 300 401
 *
 * A DAB label consists of:
 * - 16-character fixed-length label (full label)
 * - 16-bit character flag for short label extraction
 *
 * The character flag uses bit-mapped selection where each bit
 * corresponds to a character position in the full label.
 * Bit 15 (MSB) = position 0, Bit 0 (LSB) = position 15.
 */
struct DABLabel {
    std::array<char, 16> full_label;  // Full 16-character label
    uint16_t character_flag;          // Character flag field (16 bits)

    QString getFullLabel() const;     // Get full label as QString
    QString getShortLabel() const;    // Extract short label using character flag
    bool isValid() const;
    QString toString() const;

    // Constructor
    DABLabel() : character_flag(0) {
        full_label.fill(' ');
    }
};

/**
 * @brief Label types for different DAB entities
 */
enum class LabelType {
    ENSEMBLE_LABEL,    // FIG 1/0
    SERVICE_LABEL,     // FIG 1/1
    COMPONENT_LABEL    // FIG 1/4
};

/**
 * @brief FIG 0/1: Sub-channel Information
 *
 * Complete sub-channel organization structure per ETSI EN 300 401
 * Supports both short form (EEP) and long form (UEP) protection
 */
struct SubChannelInfo {
    uint8_t sub_channel_id;        // 6 bits - Sub-channel ID
    uint16_t start_address;        // 10 bits - CU start address in MSC
    bool short_form;               // Form flag: true=short(EEP), false=long(UEP)

    // Short form fields (EEP - Equal Error Protection)
    uint8_t table_switch;          // 1 bit - Table switch flag
    uint8_t table_index;           // 6 bits - Protection table index

    // Long form fields (UEP - Unequal Error Protection)
    uint8_t option;                // 3 bits - Protection option
    uint8_t protection_level;      // 2 bits - Protection level
    uint16_t sub_channel_size;     // 10 bits - Sub-channel size in CUs

    SubChannelInfo()
        : sub_channel_id(0)
        , start_address(0)
        , short_form(true)
        , table_switch(0)
        , table_index(0)
        , option(0)
        , protection_level(0)
        , sub_channel_size(0)
    {}

    // Calculate bitrate from table or size
    uint16_t getBitrate() const;

    // Get protection level string
    QString getProtectionLevel() const;

    // Get formatted description
    QString toString() const;
};

/**
 * @brief FIG 0/2: Service Component
 *
 * Service component mapping to sub-channels
 * Links services to their transport mechanism
 */
struct ServiceComponent {
    uint32_t service_id;           // SId (32 bits for data, 16 for programme)
    uint8_t component_id;          // SCIdS (4 bits) - Service Component ID within Service
    uint8_t sub_channel_id;        // 6 bits - References SubChannelInfo
    bool is_primary;               // Primary/Secondary component flag
    uint8_t transport_mode;        // TMId (2 bits) - Transport Mechanism ID
    uint8_t service_component_type; // Audio/Data type (6 bits) - from FIG 0/8
    bool ca_flag;                  // Conditional Access flag

    ServiceComponent()
        : service_id(0)
        , component_id(0)
        , sub_channel_id(0)
        , is_primary(true)
        , transport_mode(0)
        , service_component_type(0)
        , ca_flag(false)
    {}

    // Check if DAB+ (from ASCTy field via FIG 0/8)
    bool isDabPlus() const;

    // Get formatted description
    QString toString() const;
};

/**
 * @brief Complete DAB Service
 *
 * Aggregated service information from multiple FIG types
 * Combines FIG 0/2 (organization), FIG 1/1 (label), and others
 */
struct DABService {
    uint32_t service_id;           // SId - Service Identifier
    QString service_label;         // From FIG 1/1 (16 characters)
    QString short_label;           // Abbreviated label
    uint8_t service_type;          // 0=Programme(audio), 1=Data service
    bool is_dab_plus;              // DAB vs DAB+ (from FIG 0/8)
    std::vector<ServiceComponent> components;  // Service components
    uint8_t programme_type;        // PTy - Programme Type code
    uint8_t language;              // Language code
    bool ca_flag;                  // Conditional Access flag

    DABService()
        : service_id(0)
        , service_type(0)
        , is_dab_plus(false)
        , programme_type(0)
        , language(0)
        , ca_flag(false)
    {}

    // Get primary service component
    ServiceComponent getPrimaryComponent() const;

    // Get formatted description
    QString toString() const;
};

/**
 * @brief Advanced FIG (Fast Information Group) Analyzer
 *
 * Phase 1 Week 2: Complete FIG Type 0 processing (0/0, 0/1, 0/2)
 * Phase 1 Week 2: Complete FIG Type 1 processing (1/0, 1/1, 1/4)
 *
 * Supports complete ETSI EN 300 799 FIG processing:
 * - FIG 0/0: Ensemble Organization
 * - FIG 0/1: Subchannel Organization
 * - FIG 0/2: Service Organization
 * - FIG 1/0: Ensemble Labels
 * - FIG 1/1: Service Labels
 * - FIG 1/4: Service Component Labels
 */
class AdvancedFIGAnalyzer : public QObject
{
    Q_OBJECT

public:
    explicit AdvancedFIGAnalyzer(QObject *parent = nullptr);

    // Main analysis methods
    void analyzeFICData(const QByteArray& ficData);
    void reset();

    // FIG processing control
    void enableFIGType(uint8_t type, uint8_t extension, bool enabled);
    bool isFIGTypeEnabled(uint8_t type, uint8_t extension) const;

    // Data accessors - Legacy compatibility
    EnsembleInfo getCurrentEnsemble() const { return m_currentEnsemble; }
    QList<ServiceInfo> getDiscoveredServices() const;  // Convert to legacy format
    QStringList getServiceLabels() const;
    QStringList getProgrammeInfo() const;

    // New FIG Type 0 data accessors
    SubChannelInfo getSubChannelInfo(uint8_t sub_channel_id) const;
    std::vector<SubChannelInfo> getAllSubChannels() const;
    std::vector<DABService> getDABServices() const;
    DABService getService(uint32_t service_id) const;

    // Analysis results
    int getTotalFIGsProcessed() const { return m_totalFIGsProcessed; }
    int getValidFIGsCount() const { return m_validFIGsCount; }
    int getServiceCount() const;
    int getSubChannelCount() const;
    QString getLastError() const { return m_lastError; }

signals:
    // Service discovery signals
    void serviceDiscovered(const ServiceInfo& service);
    void ensembleUpdated(const EnsembleInfo& ensemble);
    void serviceLabelUpdated(uint32_t serviceId, const QString& label);

    // FIG analysis signals
    void figProcessed(const FIGData& figData);
    void figError(uint8_t figType, uint8_t extension, const QString& error);

    // Programme information signals
    void programmeTypeUpdated(uint32_t serviceId, uint8_t ptyCode);
    void timeInfoUpdated(const QDateTime& currentTime);

    // Label update signals
    void ensembleLabelUpdated(const EnsembleInfo& info);
    void componentLabelUpdated(uint32_t service_id, uint8_t component_id);

    // New FIG Type 0 signals
    void ensembleInfoUpdated(const EnsembleInfo& ensemble);
    void subChannelsUpdated();
    void servicesUpdated();

private:
    // FIG type processors
    void processFIG0(const QByteArray& figData);
    void processFIG1(const QByteArray& figData);

    // FIG 0 extension processors
    void processFIG0_0(const QByteArray& data); // Ensemble organization
    void processFIG0_1(const QByteArray& data); // Subchannel organization
    void processFIG0_2(const QByteArray& data); // Service organization
    void processFIG0_3(const QByteArray& data); // Service component
    void processFIG0_8(const QByteArray& data); // Service component global definition
    void processFIG0_13(const QByteArray& data); // User application information

    // FIG 1 extension processors
    void processFIG1_0(const QByteArray& data); // Ensemble labels
    void processFIG1_1(const QByteArray& data); // Service labels
    void processFIG1_4(const QByteArray& data); // Service component labels
    void processFIG1_5(const QByteArray& data); // Data service labels

    // Helper methods
    bool validateFIGHeader(const QByteArray& data, uint8_t& type, uint8_t& length);
    QString extractDABLabel(const QByteArray& data, int offset, int length);
    uint16_t extractUInt16(const QByteArray& data, int offset);
    uint32_t extractUInt32(const QByteArray& data, int offset);
    uint8_t extractUInt8(const QByteArray& data, int offset);

    // Bit manipulation helpers
    uint16_t extractBits(const QByteArray& data, int bit_offset, int bit_count);
    bool isBitSet(uint8_t byte, int bit_position) const;

    // Label parsing helpers
    DABLabel parseLabel(const uint8_t* data, size_t offset);
    QString extractShortLabel(const std::array<char, 16>& full_label, uint16_t char_flag);
    bool isCharacterIncluded(uint16_t char_flag, int position) const;

    // Service management
    ServiceInfo* findOrCreateService(uint32_t serviceId);
    void updateServiceInformation(uint32_t serviceId, const ServiceInfo& updates);

    // ETSI compliance checking
    bool validateETSICompliance(uint8_t figType, uint8_t extension, const QByteArray& data);
    void reportComplianceError(const QString& standard, const QString& error);

private:
    // Current analysis state - Legacy format
    EnsembleInfo m_currentEnsemble;
    ServiceInfoList m_discoveredServices;
    QMap<uint32_t, QString> m_serviceLabels;
    QMap<uint32_t, QString> m_componentLabels;
    QMap<uint32_t, uint8_t> m_programmeTypes;

    // Label storage (Agent 4's work)
    QMap<uint32_t, DABLabel> m_serviceLabelData;      // Service ID -> Label
    QMap<uint32_t, DABLabel> m_componentLabelData;    // Component ID -> Label
    DABLabel m_ensembleLabel;                          // Ensemble label

    // New FIG Type 0 data storage
    std::map<uint8_t, SubChannelInfo> m_sub_channels;  // Key: sub_channel_id
    std::map<uint32_t, DABService> m_services;         // Key: service_id

    // FIG processing control
    QMap<QString, bool> m_enabledFIGTypes; // "type.extension" -> enabled

    // Statistics
    int m_totalFIGsProcessed;
    int m_validFIGsCount;
    QString m_lastError;

    // Processing state
    QByteArray m_currentFICData;
    bool m_ensembleValid;
    uint16_t m_ensembleId;
    uint8_t m_changeFlags;

    // Time information
    QDateTime m_currentDABTime;
    bool m_timeValid;

    // Default enabled FIG types
    void initializeDefaultFIGTypes();
};

/**
 * @brief FIG parsing utilities
 *
 * Static utility functions for FIG data parsing
 * and ETSI standard compliance validation
 */
class FIGParsingUtils
{
public:
    // Character set conversion for DAB labels
    static QString convertDABCharset(const QByteArray& data, int offset, int length);

    // Country and language code utilities
    static QString countryCodeToString(uint8_t countryCode);
    static QString languageCodeToString(uint8_t languageCode);

    // Service type utilities
    static QString serviceTypeToString(uint8_t serviceType);
    static bool isAudioService(uint8_t serviceType);
    static bool isDataService(uint8_t serviceType);

    // Protection level utilities
    static QString protectionLevelToString(uint8_t protectionLevel);
    static int protectionLevelToBitrate(uint8_t protectionLevel);

    // Programme type utilities
    static QString programmeTypeToString(uint8_t ptyCode);
    static QString programmeTypeToDescription(uint8_t ptyCode);

    // Time and date utilities
    static QDateTime convertDABTime(uint32_t dabTime);
    static QString formatDABTime(const QDateTime& time);

    // CRC validation
    static bool validateFIGCRC(const QByteArray& figData);
    static uint16_t calculateFIGCRC(const QByteArray& data);

    // Bitrate calculation from sub-channel table
    static uint16_t calculateBitrate(bool short_form, uint8_t table_index,
                                     uint16_t sub_channel_size, uint8_t protection_level);
};

#endif // ADVANCED_FIG_ANALYZER_H
