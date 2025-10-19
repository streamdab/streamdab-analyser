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
    uint8_t language;              // Language code (ISO 639-2)
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
 * @brief FIG 0/3: Service Component in Packet Mode
 * ETSI EN 300 401 Section 6.3.1
 */
struct FIG0_3_ServiceComponent {
    uint16_t service_component_id;  // SCId - 12 bits
    uint8_t subchannel_id;          // SubChId - 6 bits
    uint16_t packet_address;        // 10 bits
    uint8_t data_service_component_type; // DSCTy - 6 bits
    bool ca_flag;                   // SCCA flag
    bool datagroup_flag;            // DG flag (0=datagroups, 1=packets)
};

/**
 * @brief FIG 0/5: Service Component Language
 * ETSI EN 300 401 Section 6.3.2
 */
struct FIG0_5_Language {
    uint8_t subchannel_id;          // SubChId - 6 bits
    uint8_t language_code;          // ISO 639-2 language code
    bool short_form;                // LS flag (0=short form with SubChId, 1=long form with SCId)
};

/**
 * @brief FIG 0/6: Service Linking
 * ETSI EN 300 401 Section 8.1.9
 */
struct FIG0_6_ServiceLink {
    uint16_t linkage_set_number;    // LSN - 12 bits
    bool hard_link;                 // SH flag (0=soft, 1=hard)
    bool international;             // ILS flag (0=national, 1=international)
    bool link_actuator;             // LA flag
    std::vector<uint32_t> linked_service_ids;  // List of linked service IDs
    uint8_t id_list_qualifier;      // IdLQ (0=DAB, 1=RDS, 3=DRM/AMSS)
};

/**
 * @brief FIG 0/7: Configuration Information
 * ETSI EN 300 401 Section 8.1.10
 */
struct FIG0_7_ConfigInfo {
    uint8_t service_count;          // Number of services in ensemble
    uint16_t reconfiguration_count; // 10-bit reconfiguration counter
};

/**
 * @brief FIG 0/8: Service Component Global Definition
 * ETSI EN 300 401 Section 6.3.5
 */
struct FIG0_8_GlobalDefinition {
    uint32_t service_id;            // SId (16-bit or 32-bit)
    uint8_t service_component_id;   // SCIdS - 4 bits
    bool long_form;                 // LS flag (0=short with SubChId, 1=long with SCId)
    uint8_t subchannel_id;          // SubChId (short form) or SCId (long form)
    uint16_t service_component_global_id; // SCId for long form (12 bits)
    bool is_programme_service;      // Derived from P/D flag
};

/**
 * @brief FIG 0/9: Country, LTO, International Table
 * ETSI EN 300 401 Section 5.2.2
 */
struct FIG0_9_CountryLTO {
    int8_t ensemble_lto;            // Local Time Offset in half-hours (-11.5 to +12)
    uint8_t ensemble_ecc;           // Extended Country Code
    uint8_t international_table_id; // International table ID
    bool has_extended_fields;       // ext flag
};

/**
 * @brief FIG 0/10: Date and Time
 * ETSI EN 300 401 Section 8.1.3.2
 */
struct FIG0_10_DateTime {
    uint32_t mjd;                   // Modified Julian Date (17 bits)
    uint8_t hours;                  // 0-23 (5 bits)
    uint8_t minutes;                // 0-59 (6 bits)
    uint8_t seconds;                // 0-59 (6 bits)
    uint16_t milliseconds;          // 0-999 (10 bits)
    bool lsi_flag;                  // Leap Second Indicator
    bool conf_flag;                 // Confidence flag
    bool utc_flag;                  // UTC flag
};

/**
 * @brief Advanced FIG (Fast Information Group) Analyzer
 *
 * Phase 1 Week 2: Complete FIG Type 0 processing (0/0, 0/1, 0/2)
 * Phase 1 Week 2: Complete FIG Type 1 processing (1/0, 1/1, 1/4)
 * Extended: FIG Type 0 extensions (0/3, 0/5, 0/6, 0/7, 0/8, 0/9, 0/10)
 *
 * Supports complete ETSI EN 300 799 FIG processing:
 * - FIG 0/0: Ensemble Organization
 * - FIG 0/1: Subchannel Organization
 * - FIG 0/2: Service Organization
 * - FIG 0/3: Service Component in Packet Mode
 * - FIG 0/5: Service Component Language
 * - FIG 0/6: Service Linking
 * - FIG 0/7: Configuration Information
 * - FIG 0/8: Service Component Global Definition
 * - FIG 0/9: Country, LTO, International Table
 * - FIG 0/10: Date and Time
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

    // FIG 0/3 - 0/10 data accessors
    std::vector<FIG0_3_ServiceComponent> getPacketModeComponents() const;
    std::vector<FIG0_5_Language> getComponentLanguages() const;
    std::vector<FIG0_6_ServiceLink> getServiceLinks() const;
    FIG0_7_ConfigInfo getConfigurationInfo() const;
    std::vector<FIG0_8_GlobalDefinition> getGlobalDefinitions() const;
    FIG0_9_CountryLTO getCountryLTO() const;
    FIG0_10_DateTime getDateTime() const;

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
    void processFIG0_0(const QByteArray& data);  // Ensemble organization
    void processFIG0_1(const QByteArray& data);  // Subchannel organization
    void processFIG0_2(const QByteArray& data);  // Service organization
    void processFIG0_3(const QByteArray& data);  // Service component in packet mode
    void processFIG0_5(const QByteArray& data);  // Service component language
    void processFIG0_6(const QByteArray& data);  // Service linking
    void processFIG0_7(const QByteArray& data);  // Configuration information
    void processFIG0_8(const QByteArray& data);  // Service component global definition
    void processFIG0_9(const QByteArray& data);  // Country, LTO, international table
    void processFIG0_10(const QByteArray& data); // Date and time
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

    // FIG 0/3 - 0/10 data storage
    std::vector<FIG0_3_ServiceComponent> m_packet_components;
    std::map<uint8_t, FIG0_5_Language> m_component_languages;  // Key: subchannel_id
    std::vector<FIG0_6_ServiceLink> m_service_links;
    FIG0_7_ConfigInfo m_config_info;
    std::map<uint32_t, FIG0_8_GlobalDefinition> m_global_definitions;  // Key: service_id
    FIG0_9_CountryLTO m_country_lto;
    FIG0_10_DateTime m_date_time;
    bool m_date_time_valid;

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
    static QDateTime mjdToDateTime(uint32_t mjd, uint8_t hours, uint8_t minutes, uint8_t seconds);

    // CRC validation
    static bool validateFIGCRC(const QByteArray& figData);
    static uint16_t calculateFIGCRC(const QByteArray& data);

    // Bitrate calculation from sub-channel table
    static uint16_t calculateBitrate(bool short_form, uint8_t table_index,
                                     uint16_t sub_channel_size, uint8_t protection_level);
};

#endif // ADVANCED_FIG_ANALYZER_H
