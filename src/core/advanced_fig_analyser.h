#ifndef ADVANCED_FIG_ANALYSER_H
#define ADVANCED_FIG_ANALYSER_H

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
#include <memory>
#include "eti_types.h"

namespace eti {
struct AnalyserSettings;  // complete type only in advanced_fig_analyser.cpp
}

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
 *
 * Charset handling (EN 300 401 clause 8.1.13 / TS 101 756):
 * the FIG 1 label's first data byte carries the charset flag in the
 * high nibble (0 = Complete EBU Latin, 3 = UTF-8, 6 = TIS-620 Thai).
 * getFullLabel() converts according to `charset`; the character flag
 * (mask) derivation of the short label is charset-independent.
 */
struct DABLabel {
    std::array<char, 16> full_label;  // Full 16-character label (raw wire bytes)
    uint16_t character_flag;          // Character flag field (16 bits)
    uint8_t charset = 0;              // Label charset flag: 0=EBU Latin, 3=UTF-8, 6=TIS-620

    QString getFullLabel() const;     // Get full label as QString (charset-aware conversion)
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
 * 
 * NAMING CONVENTION (Priority 3):
 * - Canonical: snake_case (sub_channel_id, sub_channel_size, start_address)
 * - Use these names consistently in all new code
 * - Do NOT use: subchannel_id, subchannelId, subChannelId variants
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
    uint16_t character_flag;       // Short-label mask from FIG 1/1 (16 bits)
    uint8_t service_type;          // 0=Programme(audio), 1=Data service
    bool is_dab_plus;              // DAB vs DAB+ (from FIG 0/8)
    std::vector<ServiceComponent> components;  // Service components
    uint8_t programme_type;        // PTy - Programme Type code
    uint8_t language;              // Language code (ISO 639-2)
    bool ca_flag;                  // Conditional Access flag

    DABService()
        : service_id(0)
        , character_flag(0)
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
 * @brief FIG 0/13: User Application Information
 * ETSI EN 300 401 Section 6.3.6 (Phase 2A Week 6)
 */
struct FIG0_13_UserApp {
    uint32_t service_id;           // Service carrying the application (16 or 32-bit)
    uint8_t component_id;          // SCIdS - Service component identifier (4 bits)
    uint16_t user_app_type;        // Application type code (11 bits)
    std::vector<uint8_t> user_app_data;  // Application-specific data
};

// User Application Type Constants - ETSI EN 300 401 Table 18
constexpr uint16_t USER_APP_NOT_USED = 0x000;
constexpr uint16_t USER_APP_SPI = 0x001;           // Service Programme Information
constexpr uint16_t USER_APP_MOT_SLIDESHOW = 0x002; // MOT Slideshow
constexpr uint16_t USER_APP_MOT_BWS = 0x003;       // MOT Broadcast Web Site
constexpr uint16_t USER_APP_TPEG = 0x004;          // TPEG (Traffic & Travel)
constexpr uint16_t USER_APP_DGPS = 0x005;          // DGPS
constexpr uint16_t USER_APP_TMC = 0x006;           // TMC (Traffic Message Channel)
constexpr uint16_t USER_APP_EPG = 0x007;           // Electronic Programme Guide
constexpr uint16_t USER_APP_DAB_JAVA = 0x008;      // DAB Java
constexpr uint16_t USER_APP_JOURNALINE = 0x044;    // Journaline

/**
 * @brief FIG 0/14: FEC Sub-channel Organization
 * ETSI EN 300 401 Section 6.2.2 (Phase 2A Week 6)
 */
struct FIG0_14_FECScheme {
    uint8_t subchannel_id;         // Sub-channel identifier (6 bits)
    uint8_t fec_scheme;            // FEC scheme identifier (2 bits)
};

// FEC Scheme Values - ETSI EN 300 401 Table 33
constexpr uint8_t FEC_SCHEME_NO_FEC = 0;
constexpr uint8_t FEC_SCHEME_RS = 1;     // Reed-Solomon
constexpr uint8_t FEC_SCHEME_RS_2 = 2;   // Reed-Solomon variant
constexpr uint8_t FEC_SCHEME_RESERVED = 3;

/**
 * @brief FIG 0/17: Programme Type
 * ETSI EN 300 401 Section 8.1.5 (Phase 2A Week 6)
 */
struct FIG0_17_ProgrammeType {
    uint16_t service_id;           // Service identifier (16-bit for programme services)
    bool is_static;                // SD flag: 0=static/dynamic PTy, 1=static PTy
    uint8_t programme_type;        // PTy code (5 bits)
};

// Programme Type Codes - ETSI EN 300 401 Table 10
constexpr uint8_t PTY_NO_PROGRAMME = 0;
constexpr uint8_t PTY_NEWS = 1;
constexpr uint8_t PTY_CURRENT_AFFAIRS = 2;
constexpr uint8_t PTY_INFORMATION = 3;
constexpr uint8_t PTY_SPORT = 4;
constexpr uint8_t PTY_EDUCATION = 5;
constexpr uint8_t PTY_DRAMA = 6;
constexpr uint8_t PTY_CULTURE = 7;
constexpr uint8_t PTY_SCIENCE = 8;
constexpr uint8_t PTY_VARIED = 9;
constexpr uint8_t PTY_POP_MUSIC = 10;
constexpr uint8_t PTY_ROCK_MUSIC = 11;
constexpr uint8_t PTY_EASY_LISTENING = 12;
constexpr uint8_t PTY_LIGHT_CLASSICAL = 13;
constexpr uint8_t PTY_SERIOUS_CLASSICAL = 14;
constexpr uint8_t PTY_OTHER_MUSIC = 15;
constexpr uint8_t PTY_WEATHER = 16;
constexpr uint8_t PTY_FINANCE = 17;
constexpr uint8_t PTY_CHILDREN = 18;
constexpr uint8_t PTY_SOCIAL_AFFAIRS = 19;
constexpr uint8_t PTY_RELIGION = 20;
constexpr uint8_t PTY_PHONE_IN = 21;
constexpr uint8_t PTY_TRAVEL = 22;
constexpr uint8_t PTY_LEISURE = 23;
constexpr uint8_t PTY_JAZZ_MUSIC = 24;
constexpr uint8_t PTY_COUNTRY_MUSIC = 25;
constexpr uint8_t PTY_NATIONAL_MUSIC = 26;
constexpr uint8_t PTY_OLDIES_MUSIC = 27;
constexpr uint8_t PTY_FOLK_MUSIC = 28;
constexpr uint8_t PTY_DOCUMENTARY = 29;
constexpr uint8_t PTY_ALARM_TEST = 30;
constexpr uint8_t PTY_ALARM = 31;

/**
 * @brief FIG 0/18: Announcement Support
 * ETSI EN 300 401 Section 8.1.6.1 (Phase 2A Week 6)
 */
struct FIG0_18_AnnouncementSupport {
    uint16_t service_id;           // Service identifier (16-bit)
    uint16_t announcement_support_flags;  // 16-bit flags (ASu field)
    std::vector<uint8_t> cluster_ids;     // List of cluster identifiers
};

// Announcement Type Flags - ETSI EN 300 401 Table 14
constexpr uint16_t ANNOUNCEMENT_ALARM = 0x0001;           // Emergency alarm (EWS)
constexpr uint16_t ANNOUNCEMENT_ROAD_TRAFFIC = 0x0002;    // Road traffic flash
constexpr uint16_t ANNOUNCEMENT_TRANSPORT_FLASH = 0x0004; // Transport flash
constexpr uint16_t ANNOUNCEMENT_WARNING = 0x0008;         // Warning/Service
constexpr uint16_t ANNOUNCEMENT_NEWS_FLASH = 0x0010;      // News flash
constexpr uint16_t ANNOUNCEMENT_AREA_WEATHER = 0x0020;    // Area weather flash
constexpr uint16_t ANNOUNCEMENT_EVENT = 0x0040;           // Event announcement
constexpr uint16_t ANNOUNCEMENT_SPECIAL_EVENT = 0x0080;   // Special event
constexpr uint16_t ANNOUNCEMENT_PROGRAMME_INFO = 0x0100;  // Programme information
constexpr uint16_t ANNOUNCEMENT_SPORT_REPORT = 0x0200;    // Sport report
constexpr uint16_t ANNOUNCEMENT_FINANCIAL_REPORT = 0x0400; // Financial report

/**
 * @brief FIG 0/19: Announcement Switching (EWS CRITICAL)
 * ETSI EN 300 401 Section 8.1.6.2 (Phase 2A Week 6)
 */
struct FIG0_19_AnnouncementSwitching {
    uint8_t cluster_id;            // Cluster identifier
    uint16_t announcement_flags;    // Active announcement types (16-bit flags - ASw field)
    uint8_t subchannel_id;         // Sub-channel carrying announcement (6 bits)
    bool new_flag;                 // New announcement indicator
    bool region_flag;              // Region flag (indicates regionId_lower present)
    uint8_t region_id;             // Geographic region (optional, 6 bits)
};

/**
 * @brief FIG 1/5: Data Service Label
 * ETSI EN 300 401 Section 5.2.2.6 (Phase 2A Week 7)
 *
 * Provides 16-character labels for data services with 32-bit Service IDs
 */
struct FIG1_5_DataServiceLabel {
    uint32_t service_id;           // 32-bit data service ID
    char label[17];                // 16 characters + null terminator
    uint16_t character_flag;       // Character field definition (16 bits)
};

/**
 * @brief FIG Type 2: Extended Label Segment
 * ETSI EN 300 401 Section 8.1.13.1 (Phase 2A Week 7)
 *
 * Reusable structure for FIG Type 2 extended labels
 * Segments are reassembled to form complete UTF-8 labels (up to 128 bytes)
 */
struct ExtendedLabelSegment {
    uint8_t toggle_flag;            // Toggle bit (0 or 1) - indicates label version
    uint8_t segment_index;          // Segment index (0-7)
    std::vector<uint8_t> text_segment;  // UTF-8 text segment (variable length, typically 16 bytes)
};

/**
 * @brief FIG 2/0: Ensemble Extended Label
 * ETSI EN 300 401 Section 8.1.13.1 (Phase 2A Week 7)
 */
struct FIG2_0_EnsembleExtLabel {
    uint16_t ensemble_id;          // EId - Ensemble identifier
    std::map<uint8_t, ExtendedLabelSegment> segments;  // segment_index → segment
};

/**
 * @brief FIG 2/1: Service Extended Label
 * ETSI EN 300 401 Section 8.1.13.1 (Phase 2A Week 7)
 */
struct FIG2_1_ServiceExtLabel {
    uint32_t service_id;           // SId - Service identifier (16 or 32-bit)
    std::map<uint8_t, ExtendedLabelSegment> segments;  // segment_index → segment
};

/**
 * @brief FIG 2/4: Service Component Extended Label
 * ETSI EN 300 401 Section 8.1.13.1 (Phase 2A Week 7)
 */
struct FIG2_4_ComponentExtLabel {
    uint32_t service_id;           // Service identifier
    uint8_t component_id;          // SCIdS (4 bits)
    std::map<uint8_t, ExtendedLabelSegment> segments;  // segment_index → segment
};

/**
 * @brief Advanced FIG (Fast Information Group) Analyser
 *
 * Phase 1 Week 2: Complete FIG Type 0 processing (0/0, 0/1, 0/2)
 * Phase 1 Week 2: Complete FIG Type 1 processing (1/0, 1/1, 1/4)
 * Phase 2A Week 5: FIG Type 0 extensions (0/3, 0/5, 0/6, 0/7, 0/8, 0/9, 0/10)
 * Phase 2A Week 6: FIG Type 0 advanced (0/13, 0/14, 0/17, 0/18, 0/19)
 * Phase 2A Week 7: FIG Type 1/5 and FIG Type 2 (Extended Labels)
 *
 * Supports complete ETSI EN 300 401 FIG processing:
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
 * - FIG 0/13: User Application Information
 * - FIG 0/14: FEC Sub-channel Organization
 * - FIG 0/17: Programme Type
 * - FIG 0/18: Announcement Support
 * - FIG 0/19: Announcement Switching (EWS)
 * - FIG 1/0: Ensemble Labels
 * - FIG 1/1: Service Labels
 * - FIG 1/4: Service Component Labels
 * - FIG 1/5: Data Service Labels
 * - FIG 2/0: Ensemble Extended Labels
 * - FIG 2/1: Service Extended Labels
 * - FIG 2/4: Component Extended Labels
 * - FIG 2/5: Data Service Extended Labels
 */
class AdvancedFIGAnalyser : public QObject
{
    Q_OBJECT

public:
    explicit AdvancedFIGAnalyser(QObject *parent = nullptr);
    ~AdvancedFIGAnalyser() override;  // out-of-line: m_settings needs a complete type

    // Main analysis methods
    void analyzeFICData(const QByteArray& ficData);
    void reset();

    // Analyser decode settings (option-variant matrix; defaults = strict FIB
    // CRC + follow-stream charset when not wired). Defined in the .cpp so
    // TUs without the complete AnalyserSettings type still compile.
    void setAnalyserSettings(const eti::AnalyserSettings& settings);
    bool hasAnalyserSettings() const { return m_settings != nullptr; }
    const eti::AnalyserSettings* analyserSettings() const { return m_settings.get(); }

    // FIB CRC / FIC decode diagnostics
    int getFibCrcFailureCount() const { return m_fib_crc_failures; }
    bool rawFicFallbackUsed() const { return m_rawFicFallbackUsed; }

    // Size in bytes of the most recently analyzed FIC (0 before the first
    // analyzeFICData() call). Fed into the GUI FIC overview panel.
    int getLastFicSize() const { return m_lastFicSize; }

    // --- FIC-XTractor: chronological per-frame FIG instance collector ---
    // Recorded during analyzeFICData() for every FIG block with a valid
    // header (type 0-2), in wire order. Purely additive: decode behaviour is
    // unchanged. The GUI wires this into Tab 3 (FIG Instance List).
    struct FigInstance {
        int frame = -1;        // ETI frame number (0 when unknown/raw stream)
        uint8_t type = 0;      // FIG type (0-6; 7 = padding, never recorded)
        uint8_t ext = 0;       // FIG extension (field width per type)
        int length = 0;        // FIG data length in bytes (after the header)
        int fibIdx = -1;       // FIB index inside the FIC (-1 = raw stream)
        bool crcOk = true;     // owning FIB's CRC-16 passed
        bool reconfig = false; // MCI reconfiguration (FIG 0/1, 0/2 C/N flag)
        QByteArray raw;        // FIG data bytes (after header, incl. ext byte)
    };

    // Per FIG type/extension block counters (observed FIGs, all frames).
    struct FigTypeCount {
        uint8_t type = 0;
        uint8_t ext = 0;
        int count = 0;
    };

    // Sets the ETI frame number for the NEXT analyzeFICData() call so the
    // recorded FigInstance::frame is the real on-air frame (GUI path).
    void setFigFrameNumber(int frame) { m_figFrameNumber = frame; }

    const QVector<FigInstance>& figInstances() const { return m_figInstances; }
    QVector<FigTypeCount> getFigTypeCounts() const;

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

    // FIG 0/13 - 0/19 data accessors (Phase 2A Week 6)
    std::vector<FIG0_13_UserApp> getUserApplications() const;
    std::vector<FIG0_14_FECScheme> getFECSchemes() const;
    std::vector<FIG0_17_ProgrammeType> getProgrammeTypes() const;
    std::vector<FIG0_18_AnnouncementSupport> getAnnouncementSupport() const;
    std::vector<FIG0_19_AnnouncementSwitching> getActiveAnnouncements() const;

    // FIG 1/5 and FIG Type 2 data accessors (Phase 2A Week 7)
    std::vector<FIG1_5_DataServiceLabel> getDataServiceLabels() const;
    QString getEnsembleExtendedLabel(uint16_t ensemble_id);
    QString getServiceExtendedLabel(uint32_t service_id);
    QString getComponentExtendedLabel(uint32_t service_id, uint8_t component_id);

    // FIG Type 2 extended label map accessors (Phase 2C - GUI)
    const std::map<uint16_t, FIG2_0_EnsembleExtLabel>& getEnsembleExtendedLabelsMap() const;
    const std::map<uint32_t, FIG2_1_ServiceExtLabel>& getServiceExtendedLabelsMap() const;
    const std::map<uint64_t, FIG2_4_ComponentExtLabel>& getComponentExtendedLabelsMap() const;

    // Analysis results
    int getTotalFIGsProcessed() const { return m_totalFIGsProcessed; }
    int getValidFIGsCount() const { return m_validFIGsCount; }
    int getServiceCount() const;
    int getSubChannelCount() const;
    QString getLastError() const { return m_lastError; }

    // Monotonic revision of the service/component/label state. Bumped on every
    // mutation of m_services / m_sub_channels / the label maps. Consumers that
    // cache derived views (e.g. the GUI sub-channel -> service map) must
    // invalidate when this changes: service *counts* alone miss late FIG 1/1
    // labels and equal-count mid-stream reconfigurations. Strictly monotonic
    // (reset() bumps it too, never zeroes it).
    uint32_t getServiceRevision() const { return m_serviceRevision; }

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

    // Phase 2A Week 6: EWS and announcement signals
    void emergencyAlarmActivated(const FIG0_19_AnnouncementSwitching& announcement);
    void announcementActivated(uint8_t cluster_id, uint16_t announcement_flags);
    void announcementDeactivated(uint8_t cluster_id);

    // Phase 2A Week 7: Extended label signals
    void extendedLabelUpdated(uint32_t entity_id, const QString& label);

private:
    // FIG type processors
    void processFIG0(const QByteArray& figData);
    void processFIG1(const QByteArray& figData);
    void processFIG2(const QByteArray& figData);  // NEW: FIG Type 2 dispatcher

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
    void processFIG0_14(const QByteArray& data); // FEC sub-channel organization
    void processFIG0_17(const QByteArray& data); // Programme type
    void processFIG0_18(const QByteArray& data); // Announcement support
    void processFIG0_19(const QByteArray& data); // Announcement switching (EWS)

    // FIG 1 extension processors
    void processFIG1_0(const QByteArray& data); // Ensemble labels
    void processFIG1_1(const QByteArray& data); // Service labels
    void processFIG1_4(const QByteArray& data); // Service component labels
    void processFIG1_5(const QByteArray& data); // Data service labels (NEW)

    // FIG 2 extension processors (NEW - Phase 2A Week 7)
    void processFIG2_0(const QByteArray& data); // Ensemble extended labels
    void processFIG2_1(const QByteArray& data); // Service extended labels
    void processFIG2_4(const QByteArray& data); // Component extended labels
    void processFIG2_5(const QByteArray& data); // Data service extended labels

    // Helper methods
    QString extractDABLabel(const QByteArray& data, int offset, int length);
    uint16_t extractUInt16(const QByteArray& data, int offset);
    uint32_t extractUInt32(const QByteArray& data, int offset);
    uint8_t extractUInt8(const QByteArray& data, int offset);

    // Bit manipulation helpers
    uint16_t extractBits(const QByteArray& data, int bit_offset, int bit_count);
    bool isBitSet(uint8_t byte, int bit_position) const;

    // Label parsing helpers
    DABLabel parseLabel(const uint8_t* data, size_t offset, uint8_t charset);
    QString extractShortLabel(const std::array<char, 16>& full_label, uint16_t char_flag);
    bool isCharacterIncluded(uint16_t char_flag, int position) const;

    // FIC / FIB walking helpers
    void walkFigArea(const uint8_t* area, size_t area_size);
    bool validateFibCrc(const uint8_t* fib) const;
    uint8_t effectiveLabelCharset(uint8_t streamCharset) const;

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

    // FIG 0/13 - 0/19 data storage (Phase 2A Week 6)
    std::vector<FIG0_13_UserApp> m_user_applications;
    std::map<uint8_t, FIG0_14_FECScheme> m_fec_schemes;  // Key: subchannel_id
    std::map<uint16_t, FIG0_17_ProgrammeType> m_programme_type_info;  // Key: service_id
    std::vector<FIG0_18_AnnouncementSupport> m_announcement_support;
    std::vector<FIG0_19_AnnouncementSwitching> m_active_announcements;

    // FIG 1/5 and FIG Type 2 data storage (Phase 2A Week 7)
    std::map<uint32_t, FIG1_5_DataServiceLabel> m_data_service_labels;  // Key: service_id
    std::map<uint16_t, FIG2_0_EnsembleExtLabel> m_ensemble_extended_labels;  // Key: ensemble_id
    std::map<uint32_t, FIG2_1_ServiceExtLabel> m_service_extended_labels;  // Key: service_id
    std::map<uint64_t, FIG2_4_ComponentExtLabel> m_component_extended_labels;  // Key: (service_id << 8) | component_id

    // FIG processing control
    QMap<QString, bool> m_enabledFIGTypes; // "type.extension" -> enabled

    // Statistics
    int m_totalFIGsProcessed;
    int m_validFIGsCount;
    QString m_lastError;

    // Monotonic service/component/label revision (see getServiceRevision()).
    // Bumped by mutating FIG processors and reset(); wrapped through the small
    // helper below so every site is consistent.
    uint32_t m_serviceRevision = 0;
    void bumpServiceRevision() { ++m_serviceRevision; }

    // FIC decode diagnostics (FIB CRC failures / raw fallback)
    int m_fib_crc_failures = 0;
    bool m_rawFicFallbackUsed = false;
    bool m_rawFicFallbackWarned = false;
    int m_lastFicSize = 0;      // bytes of the most recently analyzed FIC

    // FIC-XTractor chronological FIG instance collector (see FigInstance)
    QVector<FigInstance> m_figInstances;
    std::map<uint16_t, int> m_figTypeCounts;  // key = (type << 8) | ext
    int m_figFrameNumber = 0;        // frame for the next analyzeFICData call
    int m_figFibIndex = -1;          // current FIB index while walking
    bool m_figFibCrcOk = true;       // current FIB CRC verdict while walking
    bool m_figRecording = true;      // false during raw-fallback re-walk
    static constexpr int MAX_FIG_INSTANCES = 200000;  // GUI memory bound

    // User-selected decode options; empty = historical strict defaults
    std::unique_ptr<eti::AnalyserSettings> m_settings;

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

#endif // ADVANCED_FIG_ANALYSER_H
