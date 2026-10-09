/**
 * @file fig_parser.h
 * @brief Comprehensive ETSI-Compliant FIG Parser Engine
 *
 * Implements complete Fast Information Group (FIG) parsing according to
 * ETSI EN 300 401 Section 5.2 and related standards. Designed for
 * high-performance real-time DAB stream analysis.
 *
 * Supported FIG Types:
 * - FIG 0/0: Ensemble information
 * - FIG 0/1: Sub-channel organization
 * - FIG 0/2: Service organization
 * - FIG 0/3: Service component in packet mode
 * - FIG 0/4: Service component with conditional access
 * - FIG 0/5: Service component language
 * - FIG 0/8: Service component global definition
 * - FIG 0/10: Date and time
 * - FIG 0/18: Announcement support
 * - FIG 0/11: Region definition
 * - FIG 0/19: Announcement switching
 * - FIG 0/21: Frequency information
 * - FIG 0/24: Other ensemble services
 * - FIG 1/0: Ensemble label
 * - FIG 1/1: Programme service label
 * - FIG 1/4: Service component label
 * - FIG 1/5: Data service label
 *
 * Performance Target: >900 FPS frame processing with <5ms FIG latency
 *
 * @author Standards Compliance Agent
 * @date September 22, 2025
 * @copyright StreamDAB Analyser Project
 */

#pragma once

#include "eti_types.hpp"
#include "analyser_settings.hpp"
#include <QObject>
#include <QString>
#include <QByteArray>
#include <QDateTime>
#include <memory>
#include <vector>
#include <unordered_map>
#include <chrono>
#include <atomic>

// Forward declarations from eti namespace
namespace eti {
    struct EtiFicField;
    struct FigBlock;
}

namespace eti::fig {

// Import types from parent eti namespace
using ::eti::EtiFicField;
using ::eti::FigBlock;

/**
 * @brief FIG 0/0 Ensemble Information Structure
 * Reference: ETSI EN 300 401 Section 8.1.1.2.1
 */
struct Fig00EnsembleInfo {
    uint16_t ensemble_id;           // EId (16-bit ensemble identifier)
    uint8_t country_id;             // Country ID (4-bit)
    uint8_t extended_country_code;  // ECC (8-bit extended country code)
    uint8_t alarm_flag;             // AL (1-bit alarm flag)
    uint8_t cif_count_high;         // CIF Count (high 2 bits)
    uint8_t cif_count_low;          // CIF Count (low 8 bits)
    bool is_valid;                  // Validation flag

    uint16_t get_cif_count() const {
        return (static_cast<uint16_t>(cif_count_high) << 8) | cif_count_low;
    }

    bool validate_country_id() const {
        return country_id != 0x0F;  // Reserved value
    }

    bool is_thai_ensemble() const {
        return country_id == 0x0E && extended_country_code == 0xE1;  // Thailand
    }
};

/**
 * @brief FIG 0/1 Sub-channel Organization Structure
 * Reference: ETSI EN 300 401 Section 8.1.1.2.2
 */
struct Fig01SubchannelInfo {
    enum ProtectionType {
        UEP = 0,  // Unequal Error Protection
        EEP = 1   // Equal Error Protection
    };

    uint8_t subchannel_id;          // SubChId (6-bit)
    uint16_t start_address;         // Start Address (10-bit in CUs)
    bool short_form;                // Form flag (1=short, 0=long)

    // Short form fields
    uint8_t table_switch;           // Table Switch (1-bit)
    uint8_t table_index;            // Table Index (6-bit)

    // Long form fields
    uint8_t option;                 // Option (3-bit)
    uint8_t protection_level;       // Protection Level (2-bit)
    uint16_t subchannel_size;       // Sub-channel Size (10-bit in CUs)

    bool is_valid;                  // Validation flag

    bool validate_subchannel_id() const {
        return subchannel_id < 64;  // Maximum 64 sub-channels
    }

    bool validate_boundaries() const {
        return start_address < 864 && // Maximum CUs per frame
               (short_form || (start_address + subchannel_size) <= 864);
    }

    uint8_t get_protection_level() const {
        return short_form ? (table_index & 0x3F) : protection_level;
    }
};

/**
 * @brief FIG 0/2 Service Organization Structure
 * Reference: ETSI EN 300 401 Section 8.1.1.2.3
 */
struct Fig02ServiceInfo {
    uint32_t service_id;            // SId (16-bit or 32-bit)
    uint8_t country_id;             // Country ID (4-bit)
    uint8_t extended_country_code;  // ECC (8-bit)
    bool local_flag;                // Local flag
    bool caid_flag;                 // CAID flag
    uint8_t number_of_components;   // Number of service components

    struct ServiceComponent {
        uint8_t transport_mechanism_id; // TMId (2-bit)
        uint8_t audio_service_type;     // ASCTy/DSCTy (6-bit)
        uint8_t subchannel_id;          // SubChId (6-bit)
        uint16_t packet_address;        // Packet Address (10-bit)
        bool primary_flag;              // P/S flag
        bool ca_flag;                   // CA flag
        bool is_valid;

        // Default constructor to initialize all members
        ServiceComponent()
            : transport_mechanism_id(0), audio_service_type(0), subchannel_id(0),
              packet_address(0), primary_flag(false), ca_flag(false), is_valid(false) {}
    };

    std::vector<ServiceComponent> components;
    bool is_valid;

    // Default constructor to initialize all members
    Fig02ServiceInfo()
        : service_id(0), country_id(0), extended_country_code(0),
          local_flag(false), caid_flag(false), number_of_components(0), is_valid(false) {}

    bool validate_service_id() const {
        return service_id != 0;
    }

    bool validate_components() const {
        return number_of_components == components.size() &&
               number_of_components <= 12;  // Maximum components per service
    }

    bool is_audio_service() const {
        return std::any_of(components.begin(), components.end(),
            [](const ServiceComponent& comp) {
                return comp.transport_mechanism_id == 0;  // Stream mode
            });
    }
};

/**
 * @brief FIG 0/10 Date and Time Structure
 * Reference: ETSI EN 300 401 Section 8.1.7
 */
struct DateAndTime {
    uint32_t mjd{0};                  // Modified Julian Date (17-bit)
    uint8_t hour{0};                  // Hour (0-23)
    uint8_t minute{0};                // Minute (0-59)
    uint8_t second{0};                // Second (0-59, optional)
    bool lsi{false};                  // Leap Second Indicator
    bool conf_ind{false};             // Confidence Indicator
    bool utc_flag{false};             // UTC flag
    QDateTime datetime;               // Converted date/time
    bool is_valid{false};

    // Conversion utilities
    static QDateTime mjdToDateTime(uint32_t mjd, uint8_t hour, uint8_t minute, uint8_t second = 0);
    static uint32_t dateTimeToMjd(const QDateTime& dt);

    // Validation helpers
    bool validateTimeRanges() const {
        return hour < 24 && minute < 60 && second < 60;
    }

    bool validateMjd() const {
        return mjd <= 131071; // 17-bit maximum
    }
};

/**
 * @brief FIG 0/9 Country, LTO, and International Table Structure
 * Reference: ETSI EN 300 401 Section 8.1.8
 *
 * Provides ensemble country identification, local time offset, and character encoding table.
 * Essential for proper time zone handling and character set interpretation.
 */
struct CountryLTOInfo {
    bool ext_flag{false};              // Extension flag (1 bit)
    int8_t lto{0};                     // Local Time Offset in half-hour units (5/6 bits, signed)
    uint8_t ecc{0};                    // Extended Country Code (8 bits)
    uint8_t international_table_id{0}; // Character encoding table ID (8 bits)
    bool is_valid{false};              // Validation flag

    /**
     * @brief Convert LTO to hours (floating point)
     * @return LTO in hours (e.g., +7.0 for Thailand, -5.5 for India)
     */
    float getLTOHours() const {
        return lto * 0.5f;
    }

    /**
     * @brief Get LTO in minutes
     * @return LTO in minutes (e.g., +420 for Thailand UTC+7)
     */
    int16_t getLTOMinutes() const {
        return static_cast<int16_t>(lto) * 30;
    }

    /**
     * @brief Validate LTO range
     * LTO range: -12.0 to +14.0 hours (-24 to +28 in half-hour units)
     * @return true if LTO within valid range
     */
    bool validateLTO() const {
        return lto >= -24 && lto <= 28;
    }

    /**
     * @brief Check if this is Thai ensemble
     * @return true if ECC indicates Thailand (0xE1)
     */
    bool isThaiEnsemble() const {
        return ecc == 0xE1;
    }
};

/**
 * @brief FIG 0/19 Announcement Switching Structure
 * Reference: ETSI EN 300 401 Section 8.1.12
 *
 * Provides information about announcement switching for clustered announcement services.
 * Works in conjunction with FIG 0/18 to enable dynamic announcement switching.
 */
/**
 * @brief FIG 0/18 Announcement Support Structure
 * Reference: ETSI EN 300 401 Section 8.1.11
 *
 * Provides information about announcement types supported by a service.
 * Essential for Emergency Warning System (EWS) functionality.
 */
struct AnnouncementSupport {
    uint32_t service_id{0};           // Service ID (24-bit for DAB+, 16-bit for DAB)
    uint16_t asu_flags{0};            // Announcement Support flags (16-bit)
    uint8_t cluster_count{0};         // Number of clusters
    std::vector<uint8_t> cluster_ids; // Cluster IDs
    bool is_valid{false};

    /**
     * @brief Check if service supports a specific announcement type
     * @param type Announcement type (0-15)
     * @return true if supported
     */
    bool supportsAnnouncementType(uint8_t type) const {
        if (type > 15) return false;
        return (asu_flags & (1 << type)) != 0;
    }

    /**
     * @brief Get list of all supported announcement types
     * @return Vector of supported type IDs
     */
    std::vector<uint8_t> getSupportedTypes() const {
        std::vector<uint8_t> types;
        for (uint8_t i = 0; i < 16; ++i) {
            if (supportsAnnouncementType(i)) {
                types.push_back(i);
            }
        }
        return types;
    }

    /**
     * @brief Check if service supports emergency announcements
     * Emergency types: 0-6 (Alarm, Traffic, Transport, Warning, News, Weather, Event)
     * @return true if any emergency type supported
     */
    bool isEmergencyCapable() const {
        // Emergency types: 0-4 (Alarm, Road Traffic, Transport, Warning, News Flash)
        for (uint8_t i = 0; i <= 4; ++i) {
            if (supportsAnnouncementType(i)) return true;
        }
        return false;
    }
};

struct AnnouncementSwitching {
    uint8_t cluster_id{0};            // Cluster ID (8-bit)
    uint16_t asw_flags{0};            // Announcement Switching flags (16-bit)
    bool new_flag{false};             // New announcement flag
    bool region_flag{false};          // Regionalization flag
    uint8_t subchannel_id{0};         // Sub-channel ID (6-bit)
    std::vector<uint8_t> region_ids;  // Region IDs (if region_flag set)
    bool is_valid{false};

    /**
     * @brief Check if any announcement is active
     * @return true if ASw flags indicate active announcements
     */
    bool isActive() const {
        return asw_flags != 0;
    }

    /**
     * @brief Get list of active announcement types
     * @return Vector of announcement type indices (0-15)
     */
    std::vector<uint8_t> getActiveTypes() const {
        std::vector<uint8_t> types;
        for (uint8_t i = 0; i < 16; ++i) {
            if (asw_flags & (1 << i)) {
                types.push_back(i);
            }
        }
        return types;
    }

    /**
     * @brief Check if this switching matches a specific cluster
     * @param cluster Cluster ID to match
     * @return true if cluster_id matches
     */
    bool matchesCluster(uint8_t cluster) const {
        return cluster_id == cluster;
    }
};

/**
 * @brief FIG 0/5 Service Component Language Structure
 * Reference: ETSI EN 300 401 Section 8.1.2, ETSI TS 101 756 Table 9
 *
 * Provides language information for service components (audio/data).
 * Supports both short form (MSC stream audio) and long form (packet mode).
 */
struct ServiceComponentLanguage {
    uint32_t service_id{0};        // SId (0 for short form, 16/32-bit for long form)
    uint16_t sc_ids{0};            // Service Component ID (12 bits)
    uint8_t language_code{0};      // ISO 639-2 language code (8 bits)
    bool is_short_form{true};      // True: MSC stream audio, False: packet mode
    bool is_valid{false};

    /**
     * @brief Get human-readable language name
     * @return Language name from ISO 639-2 table (e.g., "English", "Thai", "French")
     */
    std::string getLanguageName() const;
};

/**
 * @brief FIG 0/17 Programme Type Structure
 * Reference: ETSI EN 300 401 Section 8.1.5 and ETSI TS 101 756 Table 10
 *
 * Provides programme type information for service content categorization.
 * Supports both static and dynamic programme type signalling.
 */
struct ProgrammeType {
    uint32_t service_id{0};        // SId (16-bit or 32-bit based on P/D flag)
    uint8_t pty_code{0};           // Programme Type code (5 bits, 0-31)
    uint8_t language_code{0};      // Language (8 bits, for dynamic PTy only)
    bool is_static{true};          // True: static PTy, False: dynamic PTy
    bool cc_flag{false};           // Continuity Check flag
    bool is_valid{false};

    /**
     * @brief Get Programme Type name from PTy code
     * @return Human-readable programme type name per ETSI TS 101 756 Table 10
     */
    std::string getPtyName() const {
        static const std::string pty_names[32] = {
            "No PTy",               // 0
            "News",                 // 1
            "Current Affairs",      // 2
            "Information",          // 3
            "Sport",                // 4
            "Education",            // 5
            "Drama",                // 6
            "Culture",              // 7
            "Science",              // 8
            "Varied",               // 9
            "Pop Music",            // 10
            "Rock Music",           // 11
            "Easy Listening Music", // 12
            "Light Classical",      // 13
            "Serious Classical",    // 14
            "Other Music",          // 15
            "Weather/meteorology",  // 16
            "Finance/Business",     // 17
            "Children's programmes",// 18
            "Social Affairs",       // 19
            "Religion",             // 20
            "Phone In",             // 21
            "Travel",               // 22
            "Leisure",              // 23
            "Jazz Music",           // 24
            "Country Music",        // 25
            "National Music",       // 26
            "Oldies Music",         // 27
            "Folk Music",           // 28
            "Documentary",          // 29
            "Reserved",             // 30
            "Reserved"              // 31
        };

        if (pty_code < 32) {
            return pty_names[pty_code];
        }
        return "Unknown";
    }
};

/**
 * @brief FIG 0/20 Service Component Information Structure
 * Reference: ETSI EN 300 401 Section 8.1.15
 *
 * Provides service component information including CA protection and component lists.
 * Used for detailed service component configuration and conditional access signaling.
 * PDCA Week 7 - Batch 4 / Agent 30 implementation.
 */
struct ServiceComponentInfo {
    uint32_t service_id{0};               // Service ID (16-bit or 32-bit)
    uint16_t sc_id{0};                    // Service Component ID (12-bit, 0-4095)
    bool ca_flag{false};                  // Conditional Access flag
    uint8_t num_components{0};            // Number of components
    std::vector<uint8_t> component_list;  // Component list (variable length)
    bool is_valid{false};                 // Validation flag

    /**
     * @brief Get number of components in the list
     * @return Number of components
     */
    size_t getComponentCount() const {
        return component_list.size();
    }

    /**
     * @brief Check if service component is CA protected
     * @return true if conditional access is applied
     */
    bool isProtected() const {
        return ca_flag;
    }

    /**
     * @brief Validate service component ID range
     * @return true if SCId is valid (0-4095, 12-bit)
     */
    bool validateSCId() const {
        return sc_id < 4096;
    }

    /**
     * @brief Validate service ID
     * @return true if service_id is non-zero
     */
    bool validateServiceId() const {
        return service_id != 0;
    }
};

/**
 * @brief FIG 0/24 Other Ensemble Services Structure
 * Reference: ETSI EN 300 401 Section 8.1.18
 *
 * Provides information about services available in other ensembles.
 * Used for cross-ensemble service discovery and inter-ensemble linking.
 * PDCA Week 7 - Final / Agent 33 implementation.
 */
struct OEServices {
    uint16_t ensemble_id{0};      // Other Ensemble ID (16-bit)
    uint32_t service_id{0};       // Service ID (32-bit)
    bool ca_flag{false};          // Conditional Access flag (1-bit)
    bool is_valid{false};         // Validation flag

    /**
     * @brief Check if service is CA protected
     * @return true if conditional access is applied
     */
    bool isProtected() const {
        return ca_flag;
    }

    /**
     * @brief Get ensemble ID
     * @return Other ensemble ID (16-bit)
     */
    uint16_t getEnsembleId() const {
        return ensemble_id;
    }

    /**
     * @brief Validate ensemble ID
     * @return true if ensemble_id is non-zero
     */
    bool validateEnsembleId() const {
        return ensemble_id != 0;
    }

    /**
     * @brief Validate service ID
     * @return true if service_id is non-zero
     */
    bool validateServiceId() const {
        return service_id != 0;
    }
};

/**
 * @brief FIG 0/3 Service Component in Packet Mode Structure
 * Reference: ETSI EN 300 401 Section 8.1.3
 *
 * Describes service components transmitted in packet mode, used for data services.
 * PDCA Week 4 - Agent 11 implementation.
 */
struct ServiceComponentPacketMode {
    uint32_t service_id{0};        // SId (16 or 32-bit based on P/D flag)
    uint16_t sc_id{0};             // SCId: Service component identifier (12 bits)
    uint8_t dg_flag{0};            // DG flag: Data group flag (1 bit)
    uint8_t dscty{0};              // DSCTy: Data service component type (6 bits, 0-63)
    uint16_t sub_ch_id{0};         // SubChId: Sub-channel ID (6 bits, 0-63)
    uint16_t packet_address{0};    // Packet address (10 bits, 0-1023)
    bool ca_flag{false};           // CA flag: Conditional access
    bool is_valid{false};

    /**
     * @brief Get DSCTy (Data Service Component Type) name
     * @return Human-readable name for DSCTy value
     */
    std::string getDSCTyName() const {
        switch (dscty) {
            case 0: return "Reserved";
            case 1: return "TMC (Traffic Message Channel)";
            case 5: return "TPEG";
            case 24: return "MOT (Multimedia Object Transfer)";
            case 44: return "Journaline";
            case 60: return "User-defined 60";
            case 61: return "User-defined 61";
            case 62: return "User-defined 62";
            case 63: return "User-defined 63";
            default: return "Unknown DSCTy";
        }
    }
};

/**
 * @brief FIG 0/6 Service Linking Structure
 * Reference: ETSI EN 300 401 Section 8.1.15
 *
 * Provides linking information between services for seamless handover.
 * PDCA Week 4 - Agent 12 implementation.
 */
struct ServiceLinking {
    uint32_t service_id{0};                    // Linking service SId (16/32-bit)
    bool id_list_flag{false};                  // IdLQ: 0=list, 1=single
    bool la_flag{false};                       // LA: Linkage actuator
    bool sh_flag{false};                       // SH: Soft(0)/Hard(1) link
    bool iis_flag{false};                      // IIS: National(0)/International(1)
    uint16_t lsn{0};                           // LSN: Linkage set number (12 bits, 0-4095)
    std::vector<uint32_t> linked_service_ids;  // Linked service IDs
    std::vector<uint16_t> ecc_ids;             // ECC for international linking
    bool is_valid{false};

    /**
     * @brief Get linkage type description
     * @return Human-readable linkage type
     */
    std::string getLinkageType() const {
        std::string type;
        type += sh_flag ? "Hard Link" : "Soft Link";
        type += ", ";
        type += iis_flag ? "International" : "National";
        if (la_flag) type += " (Actuator Active)";
        return type;
    }
};

/**
 * @brief FIG 0/7 Service Component Stream Mode Structure
 * Reference: ETSI EN 300 401 Section 8.1.4
 *
 * Provides service component information for stream mode audio/data services.
 * Links Service Component ID to Sub-channel ID with CA and Data Group flags.
 * PDCA Week 7 - Batch 3 / Agent 23 implementation.
 */
struct ServiceComponentStreamMode {
    uint16_t service_component_id{0};
    uint8_t sub_ch_id{0};
    bool ca_flag{false};
    bool dg_flag{false};
    bool is_valid{false};

    bool isCaProtected() const { return ca_flag; }
    bool hasDataGroups() const { return dg_flag; }
    bool validateServiceComponentId() const { return service_component_id < 4096; }
    bool validateSubChannelId() const { return sub_ch_id < 64; }
};

/**
 * @brief FIG 0/8 Service Component Global Definition Structure
 * Reference: ETSI EN 300 401 Section 8.1.6
 *
 * Provides global service component definitions linking SCIdS to SubChId/FIDCId.
 * Essential for data services and packet mode component identification.
 * PDCA Week 4 - Agent 19 implementation.
 */
struct ServiceComponentGlobal {
    uint16_t sc_ids{0};          // Service Component Identifier (12-bit)
    uint8_t rfa{0};              // Reserved for future addition (4-bit)
    bool ext_flag{false};        // Extension flag
    uint16_t sc_id{0};           // Service Component Id (12-bit)
    bool ls_flag{false};         // Long/Short form flag
    bool msc_fic_flag{false};    // MSC (true) or FIC (false)
    uint8_t sub_ch_id{0};        // Sub-channel Id (6-bit, if MSC)
    uint8_t fidc_id{0};          // FIC Data Channel Id (8-bit, if FIC)
    bool is_valid{false};

    /**
     * @brief Get transport type description
     * @return Human-readable transport type
     */
    std::string getTransportType() const {
        return msc_fic_flag ? "MSC (Main Service Channel)" : "FIC (Fast Information Channel)";
    }

    /**
     * @brief Get channel ID based on transport type
     * @return SubChId if MSC, FIDCId if FIC
     */
    uint8_t getChannelId() const {
        return msc_fic_flag ? sub_ch_id : fidc_id;
    }
};

/**
 * @brief FIG 0/13 User Application Information Structure
 * Reference: ETSI EN 300 401 Section 8.1.14
 *
 * Signals user application types available on service components.
 * PDCA Week 4 - Agent 13 implementation.
 */
struct UserApplicationInfo {
    uint32_t service_id{0};               // SId (16 or 32-bit)
    uint16_t sc_ids{0};                   // SCIdS: Service component ID (12 bits)
    uint8_t ca_flag{0};                   // CA flag: Conditional access (1 bit)
    uint16_t ua_type{0};                  // UAType: User application type (11 bits, 0-2047)
    uint8_t ua_data_length{0};            // UA data length (5 bits, 0-31 bytes)
    std::vector<uint8_t> ua_data;         // Application-specific data
    bool is_valid{false};

    /**
     * @brief Get User Application Type name
     * @return Human-readable UA type name per ETSI TS 101 756 Table 16
     */
    std::string getUATypeName() const {
        switch (ua_type) {
            case 0x001: return "Not used";
            case 0x002: return "MOT SlideShow";
            case 0x004: return "TPEG";
            case 0x044: return "TPEG-SNI";
            case 0x123: return "EPG (Electronic Programme Guide)";
            case 0x441: return "Journaline";
            default: return "Unknown UA Type";
        }
    }

    bool isMOT() const { return ua_type == 0x002; }
    bool isTPEG() const { return ua_type == 0x004; }
    bool isJournaline() const { return ua_type == 0x441; }
    bool isEPG() const { return ua_type == 0x123; }
};

/**
 * @brief FIG 0/14 FEC Sub-channel Organization Structure
 * Reference: ETSI EN 300 401 Section 8.1.5
 *
 * Provides FEC (Forward Error Correction) scheme information for sub-channels.
 * Signals the FEC protection applied to sub-channel data.
 * PDCA Week 6 - Agent 21 implementation.
 */
struct FECSubchannelOrganization {
    uint8_t sub_ch_id{0};        // Sub-channel Identifier (6-bit, 0-63)
    uint8_t fec_scheme{0};       // FEC scheme (2-bit, 0-3)
    uint16_t start_address{0};   // Start address (10-bit, in CUs)
    bool short_form{false};      // Short (true) or Long (false) form
    uint8_t table_index{0};      // Short form: table index (6-bit)
    uint16_t sub_ch_size{0};     // Long form: sub-channel size (10-bit, in CUs)
    uint8_t protection_level{0}; // Long form: protection level (2-bit)
    uint8_t option{0};           // Long form: option (3-bit)
    bool is_valid{false};

    /**
     * @brief Get FEC scheme name
     * @return Human-readable FEC scheme description
     */
    std::string getFECSchemeName() const {
        switch (fec_scheme) {
            case 0: return "No FEC";
            case 1: return "FEC scheme 1 (RS)";
            case 2: return "FEC scheme 2 (Convolutional)";
            case 3: return "Reserved";
            default: return "Unknown";
        }
    }

    /**
     * @brief Validate sub-channel ID range
     * @return true if SubChId is valid (0-63)
     */
    bool validateSubChannelId() const {
        return sub_ch_id < 64;
    }

    /**
     * @brief Validate start address boundary
     * @return true if start address is within valid range (0-863)
     */
    bool validateStartAddress() const {
        return start_address < 864;  // Maximum CUs per DAB frame
    }

    /**
     * @brief Validate sub-channel size boundary (long form only)
     * @return true if size is within valid range
     */
    bool validateSubChannelSize() const {
        if (short_form) return true;  // Not applicable to short form
        return sub_ch_size > 0 && (start_address + sub_ch_size) <= 864;
    }

    /**
     * @brief Get protection level description
     * @return Protection level value (short form: from table_index, long form: protection_level)
     */
    uint8_t getProtectionLevel() const {
        return short_form ? (table_index & 0x3F) : protection_level;
    }
};

/**
 * @brief FIG 0/15 Programme Number Structure
 * Reference: ETSI EN 300 401 Section 8.1.13
 *
 * Provides programme number information for services.
 * Links services to programme numbers for EPG and content identification.
 * PDCA Week 7 - Batch 4 / Agent 28 implementation.
 */
struct ProgrammeNumber {
    uint32_t service_id{0};           // Service ID (16-bit or 32-bit)
    uint16_t programme_number{0};     // Programme Number (16-bit, 0-65535)
    bool continuation_flag{false};    // Continuation flag (1-bit)
    bool update_flag{false};          // Update flag (1-bit)
    bool is_valid{false};             // Validation flag

    /**
     * @brief Check if programme has continuation
     * @return true if continuation_flag is set
     */
    bool isContinuation() const {
        return continuation_flag;
    }

    /**
     * @brief Check if programme has updates
     * @return true if update_flag is set
     */
    bool hasUpdate() const {
        return update_flag;
    }

    /**
     * @brief Validate service ID
     * @return true if service_id is non-zero
     */
    bool validateServiceId() const {
        return service_id != 0;
    }

    /**
     * @brief Validate programme number
     * @return true if programme_number is non-zero
     */
    bool validateProgrammeNumber() const {
        return programme_number != 0;
    }
};

/**
 * @brief FIG 0/16 Programme Type International Structure
 * Reference: ETSI EN 300 401 Section 8.1.14
 *
 * Provides international programme type categorization for cross-border services.
 * Supports RDS PTy compatibility, RBDS (North America), and proprietary regional schemes.
 * PDCA Week 7 - Batch 4 / Agent 29 implementation.
 */
struct ProgrammeTypeInternational {
    uint32_t service_id{0};           // Service ID (16-bit in practice, 32-bit field)
    uint8_t international_code{0};    // International code (8-bit, 0-255)
    uint8_t programme_type{0};        // Programme type (5-bit, 0-31)
    uint8_t language_code{0};         // Language code (8-bit)
    bool is_valid{false};             // Validation flag

    /**
     * @brief Get international type name
     * @return Human-readable international code description
     */
    std::string getInternationalTypeName() const {
        if (international_code == 0x00) {
            return "RDS PTy Compatible";
        } else if (international_code == 0x01) {
            return "RBDS (North America)";
        } else if (international_code >= 0x80 && international_code <= 0xFF) {
            return "Proprietary/Regional";
        } else {
            return "Reserved";
        }
    }

    /**
     * @brief Validate international code
     * @return true (all 8-bit values are valid)
     */
    bool isValidCode() const {
        return true;  // All 8-bit values (0-255) are valid
    }

    /**
     * @brief Get programme type name
     * @return Human-readable programme type name (same as FIG 0/17)
     */
    std::string getProgrammeTypeName() const {
        static const std::string pty_names[32] = {
            "No PTy",               // 0
            "News",                 // 1
            "Current Affairs",      // 2
            "Information",          // 3
            "Sport",                // 4
            "Education",            // 5
            "Drama",                // 6
            "Culture",              // 7
            "Science",              // 8
            "Varied",               // 9
            "Pop Music",            // 10
            "Rock Music",           // 11
            "Easy Listening Music", // 12
            "Light Classical",      // 13
            "Serious Classical",    // 14
            "Other Music",          // 15
            "Weather/meteorology",  // 16
            "Finance/Business",     // 17
            "Children's programmes",// 18
            "Social Affairs",       // 19
            "Religion",             // 20
            "Phone In",             // 21
            "Travel",               // 22
            "Leisure",              // 23
            "Jazz Music",           // 24
            "Country Music",        // 25
            "National Music",       // 26
            "Oldies Music",         // 27
            "Folk Music",           // 28
            "Documentary",          // 29
            "Reserved",             // 30
            "Reserved"              // 31
        };

        if (programme_type < 32) {
            return pty_names[programme_type];
        }
        return "Unknown";
    }
};

/**
 * @brief FIG 0/11 Region Definition Structure
 * Reference: ETSI EN 300 401 Section 8.1.9
 *
 * Provides regional service information for service area definition.
 * Enables region-based service differentiation within an ensemble.
 * PDCA Week 7 - Batch 3 / Agent 24 implementation.
 */
struct RegionDefinition {
    uint8_t region_id{0};        // Region ID (8-bit, 0-255)
    bool region_flag{false};     // Region flag (1-bit): 0=SubChId, 1=EId
    uint8_t sub_ch_id{0};        // Sub-channel ID (6-bit, 0-63) if region_flag=0
    uint16_t ensemble_id{0};     // Ensemble ID (16-bit) if region_flag=1
    bool is_valid{false};        // Validation flag

    /**
     * @brief Check if region is active
     * @return true if region_flag is set (ensemble-level region)
     */
    bool isActive() const {
        return region_flag;
    }

    /**
     * @brief Get region scope description
     * @return Human-readable region scope
     */
    std::string getRegionScope() const {
        if (region_flag) {
            return "Ensemble-level (EId: 0x" +
                   std::to_string(ensemble_id) + ")";
        } else {
            return "Sub-channel level (SubChId: " +
                   std::to_string(sub_ch_id) + ")";
        }
    }

    /**
     * @brief Validate region ID range
     * @return true if region_id is valid (0-255)
     */
    bool validateRegionId() const {
        return true;  // All 8-bit values valid
    }

    /**
     * @brief Validate sub-channel ID range (if applicable)
     * @return true if SubChId is valid (0-63) when region_flag=0
     */
    bool validateSubChannelId() const {
        if (region_flag) return true;  // Not applicable
        return sub_ch_id < 64;
    }
};


/**
 * @brief FIG 0/21 Frequency Information Structure
 * Reference: ETSI EN 300 401 Section 8.1.16
 *
 * Provides alternative frequency information for DAB services.
 * Used for seamless frequency handover and service continuity.
 * PDCA Week 7 - Batch 3 / Agent 25 implementation.
 */
struct FrequencyInformation {
    uint8_t region_id{0};                  // Region ID (8-bit, optional)
    std::vector<uint32_t> frequency_list;  // Frequency list in Hz
    uint8_t length_indicator{0};           // Length indicator (number of frequencies)
    bool is_valid{false};                  // Validation flag

    /**
     * @brief Get number of frequencies in the list
     * @return Number of frequencies
     */
    size_t getFrequencyCount() const {
        return frequency_list.size();
    }

    /**
     * @brief Get frequency at specific index
     * @param index Index of frequency to retrieve
     * @return Frequency in Hz (0 if index out of range)
     */
    uint32_t getFrequencyAt(size_t index) const {
        if (index < frequency_list.size()) {
            return frequency_list[index];
        }
        return 0;
    }

    /**
     * @brief Validate frequency range (Band III: 174-240 MHz)
     * @param freq Frequency in Hz
     * @return true if frequency is within valid DAB Band III range
     */
    static bool isValidBandIIIFrequency(uint32_t freq) {
        return freq >= 174000000 && freq <= 240000000;  // 174-240 MHz
    }

    /**
     * @brief Get frequency in MHz for display
     * @param index Index of frequency
     * @return Frequency in MHz as double (e.g., 225.648)
     */
    double getFrequencyMHz(size_t index) const {
        if (index < frequency_list.size()) {
            return static_cast<double>(frequency_list[index]) / 1000000.0;
        }
        return 0.0;
    }
};

/**
 * @brief FIG 0/12 MPEG-2 TS Streaming Structure
 * Reference: ETSI EN 300 401 Section 8.1.10
 *
 * Provides MPEG-2 Transport Stream configuration for DVB services over DAB.
 * Used for hybrid DVB-T/DAB systems with MPEG-2 TS multiplexing.
 * PDCA Week 7 - Batch 4 / Agent 27 implementation.
 */
struct Mpeg2TsStreaming {
    uint32_t service_id{0};         // Service ID (32-bit)
    bool msc_flag{false};           // MSC flag: true=MSC transport, false=reserved
    uint8_t sub_ch_id{0};           // Sub-channel ID (6-bit, 0-63)
    uint16_t mpeg_frame_size{0};    // MPEG-2 TS frame size (16-bit)
    bool is_valid{false};           // Validation flag

    /**
     * @brief Check if using MSC transport
     * @return true if MSC transport is used
     */
    bool isMscTransport() const {
        return msc_flag;
    }

    /**
     * @brief Get MPEG-2 TS frame size
     * @return Frame size in bytes
     */
    uint16_t getFrameSize() const {
        return mpeg_frame_size;
    }

    /**
     * @brief Validate sub-channel ID range
     * @return true if SubChId is valid (0-63)
     */
    bool validateSubChannelId() const {
        return sub_ch_id < 64;
    }

    /**
     * @brief Validate frame size (typical MPEG-2 TS packet is 188 bytes)
     * @return true if frame size is non-zero
     */
    bool validateFrameSize() const {
        return mpeg_frame_size > 0;
    }

    /**
     * @brief Validate service ID (must be non-zero)
     * @return true if service ID is valid
     */
    bool validateServiceId() const {
        return service_id != 0;
    }
};

/**
 * @brief FIG 1/0 Ensemble Label Structure
 * Reference: ETSI EN 300 401 Section 8.1.13
 */
struct EnsembleLabel {
    uint16_t ensemble_id;           // Ensemble identifier (EId)
    std::string label;              // Ensemble label (up to 16 characters)
    uint16_t character_flag_field;  // Character flag field
    bool uses_utf8;                 // UTF-8 encoding flag
    bool supports_thai;             // Thai character support
    bool is_valid;                  // Validation flag

    bool validate_label_length() const {
        return label.length() <= 16;
    }

    bool validate_character_encoding() const {
        // Validate proper UTF-8 encoding for Thai support
        if (!supports_thai) return true;

        // Check for valid Thai UTF-8 sequences
        return std::all_of(label.begin(), label.end(), [](unsigned char c) {
            return c < 0x80 || (c >= 0xE0 && c <= 0xEF);  // Thai Unicode range
        });
    }
};

/**
 * @brief FIG 1/x Service Label Structures
 * Reference: ETSI EN 300 401 Section 8.1.2
 */
struct ServiceLabel {
    uint32_t service_id;            // Service identifier
    std::string label;              // Service label (up to 16 characters)
    uint16_t character_flag_field;  // Character flag field
    bool uses_utf8;                 // UTF-8 encoding flag
    bool supports_thai;             // Thai character support
    bool is_valid;                  // Validation flag

    bool validate_label_length() const {
        return label.length() <= 16;
    }

    bool validate_character_encoding() const {
        // Validate proper UTF-8 encoding for Thai support
        if (!supports_thai) return true;

        // Check for valid Thai UTF-8 sequences
        return std::all_of(label.begin(), label.end(), [](unsigned char c) {
            return c < 0x80 || (c >= 0xE0 && c <= 0xEF);  // Thai Unicode range
        });
    }
};

/**
 * @brief FIG Parsing Result Structure
 */
struct FigParsingResult {
    bool parsing_successful;
    std::chrono::nanoseconds parsing_time;
    uint32_t figs_processed;
    uint32_t parsing_errors;

    // Parsed FIG data
    std::vector<Fig00EnsembleInfo> ensemble_info;
    std::vector<Fig01SubchannelInfo> subchannel_info;
    std::vector<Fig02ServiceInfo> service_info;
    std::vector<EnsembleLabel> ensemble_labels;
    std::vector<ServiceLabel> service_labels;
    std::vector<DateAndTime> date_time_info;
    std::vector<AnnouncementSupport> announcement_support;
    std::vector<AnnouncementSwitching> announcement_switching;
    std::vector<ServiceComponentLanguage> component_languages;
    std::vector<ProgrammeType> programme_types;

    // ETSI compliance status
    bool etsi_compliant;
    std::vector<std::string> compliance_violations;
    std::vector<std::string> compliance_warnings;

    // FIB-level CRC statistics (number of FIBs skipped due to bad CRC)
    uint32_t fib_crc_failures;

    void reset() {
        parsing_successful = false;
        parsing_time = std::chrono::nanoseconds::zero();
        figs_processed = 0;
        parsing_errors = 0;
        ensemble_info.clear();
        subchannel_info.clear();
        service_info.clear();
        ensemble_labels.clear();
        service_labels.clear();
        date_time_info.clear();
        announcement_support.clear();
        announcement_switching.clear();
        etsi_compliant = false;
        compliance_violations.clear();
        compliance_warnings.clear();
        fib_crc_failures = 0;
    }
};

/**
 * @brief High-Performance ETSI-Compliant FIG Parser
 *
 * Implements complete FIG parsing with optimized performance for
 * real-time DAB stream analysis. Designed to maintain >900 FPS
 * processing rate while providing comprehensive ETSI compliance.
 */
class FigParser : public QObject {
    Q_OBJECT

public:
    explicit FigParser(QObject* parent = nullptr);
    ~FigParser();

    /**
     * @brief Initialize the FIG parser
     * @param enable_thai_support Enable Thai character encoding support
     * @param strict_compliance Enable strict ETSI compliance checking
     * @return true if initialization successful
     */
    bool initialize(bool enable_thai_support = true, bool strict_compliance = false);

    /**
     * @brief Set the user-selected analyser decode options (rows 4/5).
     *
     * Drives the FIC/FIB decode mode used by extractFigBlocks(). Defaults to
     * AnalyserSettings::defaults() so behavior is unchanged for default configs.
     * @param settings Analyser decode options to use
     */
    void setAnalyserSettings(const AnalyserSettings& settings) { settings_ = settings; }

    /**
     * @brief Current analyser decode options.
     */
    const AnalyserSettings& analyserSettings() const noexcept { return settings_; }

    /**
     * @brief Parse FIC data and extract all FIG blocks
     * @param fic_data FIC field data (32 bytes)
     * @return FIG parsing result with all extracted information
     */
    FigParsingResult parseFicData(const EtiFicField& fic_data);

    /**
     * @brief Parse individual FIG block
     * @param fig_block FIG block to parse
     * @return true if parsing successful
     */
    bool parseFigBlock(const FigBlock& fig_block);

    /**
     * @brief Parse FIG 0/0 ensemble information
     * @param fig_data FIG data payload
     * @return Parsed ensemble information
     */
    Fig00EnsembleInfo parseFig00_EnsembleInfo(const std::vector<uint8_t>& fig_data);

    /**
     * @brief Parse FIG 0/1 sub-channel organization
     * @param fig_data FIG data payload
     * @return Vector of parsed sub-channel information
     */
    std::vector<Fig01SubchannelInfo> parseFig01_SubchannelOrganization(
        const std::vector<uint8_t>& fig_data);

    /**
     * @brief Parse FIG 0/2 service organization
     * @param fig_data FIG data payload
     * @return Vector of parsed service information
     */
    std::vector<Fig02ServiceInfo> parseFig02_ServiceOrganization(
        const std::vector<uint8_t>& fig_data);

    /**
     * @brief Parse FIG 0/10 date and time
     * @param fig_data FIG data payload
     * @return Parsed date and time information
     */
    DateAndTime parseFig010_DateAndTime(const std::vector<uint8_t>& fig_data);

    /**
     * @brief Parse FIG 0/18 announcement support
     * @param fig_data FIG data payload
     * @return Parsed announcement support information
     */
    AnnouncementSupport parseFig018_AnnouncementSupport(const std::vector<uint8_t>& fig_data);

    /**
     * @brief Parse FIG 0/5: Service Component Language
     * @param fig_data FIG data payload
     * @return Parsed service component language information
     * Reference: ETSI EN 300 401 Section 8.1.2, ETSI TS 101 756 Table 9
     */
    ServiceComponentLanguage parseFig005_ServiceComponentLanguage(
        const std::vector<uint8_t>& fig_data);

    /**
     * @brief Parse FIG 0/17 programme type
     * @param fig_data FIG data payload
     * @return Parsed programme type information
     * Reference: ETSI EN 300 401 Section 8.1.5
     */
    ProgrammeType parseFig017_ProgrammeType(const std::vector<uint8_t>& fig_data);

    /**
     * @brief Parse FIG 0/3 service component in packet mode
     * @param fig_data FIG data payload
     * @return Parsed service component packet mode information
     * Reference: ETSI EN 300 401 Section 8.1.3
     * PDCA Week 4 - Agent 11
     */
    ServiceComponentPacketMode parseFig003_ServiceComponentPacketMode(
        const std::vector<uint8_t>& fig_data);

    /**
     * @brief Parse FIG 0/6 service linking
     * @param fig_data FIG data payload
     * @return Parsed service linking information
     * Reference: ETSI EN 300 401 Section 8.1.15
     * PDCA Week 4 - Agent 12
     */
    ServiceLinking parseFig006_ServiceLinking(const std::vector<uint8_t>& fig_data);

    ServiceComponentStreamMode parseFig007_ServiceComponentStreamMode(
        const std::vector<uint8_t>& fig_data);

    /**
     * @brief Parse FIG 0/8 service component global definition
     * @param fig_data FIG data payload
     * @return Parsed service component global definition
     * Reference: ETSI EN 300 401 Section 8.1.6
     * PDCA Week 4 - Agent 19
     */
    ServiceComponentGlobal parseFig008_ServiceComponentGlobal(
        const std::vector<uint8_t>& fig_data);

    /**
     * @brief Parse FIG 0/9 country, LTO, and international table
     * @param fig_data FIG data payload
     * @return Parsed country/LTO information
     * Reference: ETSI EN 300 401 Section 8.1.8
     * PDCA Week 6 - Agent 20
     */
    CountryLTOInfo parseFig009_CountryLTO(const std::vector<uint8_t>& fig_data);

    /**
     * @brief Parse FIG 0/11 region definition
     * @param fig_data FIG data payload
     * @return Parsed region definition information
     * Reference: ETSI EN 300 401 Section 8.1.9
     * PDCA Week 7 - Batch 3 / Agent 24
     */
    RegionDefinition parseFig011_RegionDefinition(const std::vector<uint8_t>& fig_data);

    /**
     * @brief Parse FIG 0/12 MPEG-2 TS streaming
     * @param fig_data FIG data payload
     * @return Parsed MPEG-2 TS streaming information
     * Reference: ETSI EN 300 401 Section 8.1.10
     * PDCA Week 7 - Batch 4 / Agent 27
     */

    /**
     * @brief Parse FIG 0/12 MPEG-2 TS streaming
     * @param fig_data FIG data payload
     * @return Parsed MPEG-2 TS streaming information
     * Reference: ETSI EN 300 401 Section 8.1.10
     * PDCA Week 7 - Batch 4 / Agent 27
     */
    Mpeg2TsStreaming parseFig012_Mpeg2TsStreaming(const std::vector<uint8_t>& fig_data);

    /**
     * @brief Parse FIG 0/13 user application information
     * @param fig_data FIG data payload
     * @return Parsed user application information
     * Reference: ETSI EN 300 401 Section 8.1.14
     * PDCA Week 4 - Agent 13
     */
    UserApplicationInfo parseFig013_UserApplicationInfo(
        const std::vector<uint8_t>& fig_data);

    /**
     * @brief Parse FIG 0/14 FEC sub-channel organization
     * @param fig_data FIG data payload
     * @return Parsed FEC sub-channel information
     * Reference: ETSI EN 300 401 Section 8.1.5
     * PDCA Week 6 - Agent 21
     */
    FECSubchannelOrganization parseFig014_FECSubchannel(const std::vector<uint8_t>& fig_data);

    /**
     * @brief Parse FIG 0/15 programme number
     * @param fig_data FIG data payload
     * @return Parsed programme number information
     * Reference: ETSI EN 300 401 Section 8.1.13
     * PDCA Week 7 - Batch 4 / Agent 28
     */
    ProgrammeNumber parseFig015_ProgrammeNumber(const std::vector<uint8_t>& fig_data);

    /**
     * @brief Parse FIG 0/16 programme type international
     * @param fig_data FIG data payload
     * @return Parsed programme type international information
     * Reference: ETSI EN 300 401 Section 8.1.14
     * PDCA Week 7 - Batch 4 / Agent 29
     */
    ProgrammeTypeInternational parseFig016_ProgrammeTypeInternational(
        const std::vector<uint8_t>& fig_data);

    /**
     * @brief Parse FIG 0/19 announcement switching
     * @param fig_data FIG data payload
     * @return Parsed announcement switching information
     * Reference: ETSI EN 300 401 Section 8.1.12
     */
    AnnouncementSwitching parseFig019_AnnouncementSwitching(
        const std::vector<uint8_t>& fig_data);
    /**
     * @brief Parse FIG 0/20 service component information
     * @param fig_data FIG data payload
    //      * @return Parsed service component information
    //      * Reference: ETSI EN 300 401 Section 8.1.15
          * PDCA Week 7 - Batch 4 / Agent 30
          */
    ServiceComponentInfo parseFig020_ServiceComponentInfo(
        const std::vector<uint8_t>& fig_data);


    /**
     * @brief Parse FIG 0/21 frequency information
     * @param fig_data FIG data payload
     * @return Parsed frequency information
     * Reference: ETSI EN 300 401 Section 8.1.16
     * PDCA Week 7 - Batch 3 / Agent 25
     */
    FrequencyInformation parseFig021_FrequencyInformation(
        const std::vector<uint8_t>& fig_data);

    /**
     * @brief Parse FIG 0/24 other ensemble services
     * @param fig_data FIG data payload
     * @return Parsed other ensemble services information
     * Reference: ETSI EN 300 401 Section 8.1.18
     * PDCA Week 7 - Final / Agent 33
     */
    OEServices parseFig024_OEServices(const std::vector<uint8_t>& fig_data);

    /**
     * @brief Parse FIG 1/0 ensemble label
     * @param fig_data FIG data payload
     * @return Parsed ensemble label
     */
    EnsembleLabel parseFig10_EnsembleLabel(const std::vector<uint8_t>& fig_data);

    /**
     * @brief Parse FIG 1/1 programme service labels
     * @param fig_data FIG data payload
     * @return Vector of parsed service labels
     */
    std::vector<ServiceLabel> parseFig11_ProgrammeServiceLabels(
        const std::vector<uint8_t>& fig_data);

    /**
     * @brief Validate FIG parsing against ETSI standards
     * @param result FIG parsing result to validate
     * @return true if ETSI compliant
     */
    bool validateEtsiCompliance(FigParsingResult& result);

    /**
     * @brief Get parsing performance statistics
     */
    struct ParsingStatistics {
        uint64_t total_fics_processed;
        uint64_t total_figs_processed;
        uint64_t parsing_errors;
        uint64_t ensemble_labels_processed;
        uint64_t service_labels_processed;
        std::chrono::nanoseconds total_parsing_time;
        double average_figs_per_second;
        double average_parsing_latency_ms;

        double getFigProcessingRate() const {
            if (total_parsing_time.count() == 0) return 0.0;
            return static_cast<double>(total_figs_processed) /
                   (static_cast<double>(total_parsing_time.count()) / 1e9);
        }
    };

    ParsingStatistics getStatistics() const { return statistics_; }
    void resetStatistics();

    /**
     * @brief Enable/disable specific FIG type parsing
     * @param fig_type FIG type to configure (e.g., 0x00 for FIG 0/0)
     * @param enabled Enable or disable parsing
     */
    void enableFigType(uint8_t fig_type, bool enabled);

    /**
     * @brief Check if parser is ready for processing
     * @return true if initialized and ready
     */
    bool isReady() const { return initialized_; }

signals:
    /**
     * @brief Emitted when ensemble information is discovered
     * @param ensemble_info Discovered ensemble information
     */
    void ensembleInfoDiscovered(const Fig00EnsembleInfo& ensemble_info);

    /**
     * @brief Emitted when sub-channel organization is updated
     * @param subchannel_info Vector of sub-channel information
     */
    void subchannelOrganizationUpdated(const std::vector<Fig01SubchannelInfo>& subchannel_info);

    /**
     * @brief Emitted when service is discovered
     * @param service_info Discovered service information
     */
    void serviceDiscovered(const Fig02ServiceInfo& service_info);

    /**
     * @brief Emitted when date and time is discovered
     * @param date_time Discovered date and time information
     */
    void dateTimeDiscovered(const DateAndTime& date_time);

    /**
     * @brief Emitted when announcement support is discovered
     * @param announcement Discovered announcement support information
     */
    void announcementSupportDiscovered(const AnnouncementSupport& announcement);

    /**
     * @brief Emitted when service component language is discovered
     * @param language_info Service component language information
     */
    void serviceComponentLanguageDiscovered(const ServiceComponentLanguage& language_info);

    /**
     * @brief Emitted when programme type is discovered
     * @param programme_type Discovered programme type information
     */
    void programmeTypeDiscovered(const ProgrammeType& programme_type);

    /**
     * @brief Emitted when service component packet mode is discovered
     * @param packet_mode Discovered service component packet mode information
     * PDCA Week 4 - Agent 11
     */
    void serviceComponentPacketModeDiscovered(const ServiceComponentPacketMode& packet_mode);

    /**
     * @brief Emitted when service linking is discovered
     * @param linking Discovered service linking information
     * PDCA Week 4 - Agent 12
     */
    void serviceLinkingDiscovered(const ServiceLinking& linking);

    void serviceComponentStreamModeDiscovered(const ServiceComponentStreamMode& stream_mode);

    /**
     * @brief Emitted when service component global definition is discovered
     * @param global_def Discovered service component global definition
     * PDCA Week 4 - Agent 19
     */
    void serviceComponentGlobalDiscovered(const ServiceComponentGlobal& global_def);

    /**
     * @brief Emitted when country/LTO information is discovered
     * @param country_lto Country, LTO, and international table information
     * PDCA Week 6 - Agent 20
     */
    void countryLTODiscovered(const CountryLTOInfo& country_lto);

    /**
     * @brief Emitted when FEC sub-channel organization is discovered
     * @param fec_subchannel FEC sub-channel organization information
     * PDCA Week 6 - Agent 21
     */
    void fecSubchannelDiscovered(const FECSubchannelOrganization& fec_subchannel);

    /**
     * @brief Emitted when programme number is discovered
     * @param programme_number Programme number information
     * PDCA Week 7 - Batch 4 / Agent 28
     */
    void programmeNumberDiscovered(const ProgrammeNumber& programme_number);

    /**
     * @brief Emitted when programme type international is discovered
     * @param pty_intl Programme type international information
     * PDCA Week 7 - Batch 4 / Agent 29
     */
    void programmeTypeInternationalDiscovered(const ProgrammeTypeInternational& pty_intl);

    /**
     * @brief Emitted when region definition is discovered
     * @param region_def Region definition information
     * PDCA Week 7 - Batch 3 / Agent 24
     */
    void regionDefinitionDiscovered(const RegionDefinition& region_def);

    /**
     * @brief Emitted when user application information is discovered
     * @param ua_info Discovered user application information
     * PDCA Week 4 - Agent 13
     */
    void userApplicationInfoDiscovered(const UserApplicationInfo& ua_info);

    /**
     * @brief Emitted when announcement switching is detected
     * @param announcement_switch Announcement switching information
     */
    void announcementSwitchingDetected(const AnnouncementSwitching& announcement_switch);

    /**
     * @brief Emitted when frequency information is discovered
     * @param freq_info Frequency information
     * PDCA Week 7 - Batch 3 / Agent 25
     */
    /**
     * @brief Emitted when service component information is discovered
     * @param sc_info Service component information
     * PDCA Week 7 - Batch 4 / Agent 30
     */
    void serviceComponentInfoDiscovered(const ServiceComponentInfo& sc_info);

    void frequencyInformationDiscovered(const FrequencyInformation& freq_info);

    /**
     * @brief Emitted when MPEG-2 TS streaming is discovered
     * @param mpeg2_ts MPEG-2 TS streaming information
     * PDCA Week 7 - Batch 4 / Agent 27
     */
    void mpeg2TsStreamingDiscovered(const Mpeg2TsStreaming& mpeg2_ts);

    /**
     * @brief Emitted when other ensemble services is discovered
     * @param oe_services Other ensemble services information
     * PDCA Week 7 - Final / Agent 33
     */
    void oeServicesDiscovered(const OEServices& oe_services);

    /**
     * @brief Emitted when ensemble label is discovered
     * @param ensemble_label Discovered ensemble label
     */
    void ensembleLabelDiscovered(const EnsembleLabel& ensemble_label);

    /**
     * @brief Emitted when service label is discovered
     * @param service_label Discovered service label
     */
    void serviceLabelDiscovered(const ServiceLabel& service_label);

    /**
     * @brief Emitted when ETSI compliance violation is detected
     * @param violation_description Description of the violation
     * @param fig_type FIG type where violation occurred
     */
    void complianceViolationDetected(const QString& violation_description, uint8_t fig_type);

    /**
     * @brief Emitted when Thai encoding issue is detected
     * @param issue_description Description of the encoding issue
     * @param service_id Service ID with the issue
     */
    void thaiEncodingIssueDetected(const QString& issue_description, uint32_t service_id);

private:
    // Core FIG parsing methods
    std::vector<FigBlock> extractFigBlocks(const EtiFicField& fic_data);
    bool validateFigBlockStructure(const FigBlock& fig_block);
    bool validateFigTiming(uint8_t fig_type, uint8_t extension);

    // FIG type parsing methods
    bool parseFigType0(const FigBlock& fig_block, uint8_t extension);
    bool parseFigType1(const FigBlock& fig_block, uint8_t extension);

    // FIG type-specific parsers
    bool parseFig00_Internal(const std::vector<uint8_t>& data, Fig00EnsembleInfo& result);
    bool parseFig01_Internal(const std::vector<uint8_t>& data, std::vector<Fig01SubchannelInfo>& result);
    bool parseFig02_Internal(const std::vector<uint8_t>& data, std::vector<Fig02ServiceInfo>& result);
    bool parseFig010_Internal(const std::vector<uint8_t>& data, DateAndTime& result);
    bool parseFig10_Internal(const std::vector<uint8_t>& data, EnsembleLabel& result);
    bool parseFig11_Internal(const std::vector<uint8_t>& data, std::vector<ServiceLabel>& result);

    // ETSI compliance validation
    bool validateEnsembleCompliance(const Fig00EnsembleInfo& ensemble);
    bool validateSubchannelCompliance(const Fig01SubchannelInfo& subchannel);
    bool validateServiceCompliance(const Fig02ServiceInfo& service);
    bool validateDateTimeCompliance(const DateAndTime& dt);
    bool validateAnnouncementSupportCompliance(const AnnouncementSupport& announcement);
    bool validateEnsembleLabelCompliance(const EnsembleLabel& label);
    bool validateLabelCompliance(const ServiceLabel& label);

    // Thai character encoding support
    bool validateThaiCharacterEncoding(const std::string& text);
    bool isThaiUnicodeRange(uint32_t codepoint);
    std::string normalizeThaiText(const std::string& input);

    // Performance optimization
    void updateStatistics(std::chrono::nanoseconds parsing_time, uint32_t figs_processed);
    bool shouldOptimizeForPerformance() const;
    void optimizeParsingStrategy();

private:
    bool initialized_;
    bool thai_support_enabled_;
    bool strict_compliance_;

    // FIG type enablement flags
    std::array<bool, 256> fig_types_enabled_;

    // Performance tracking
    ParsingStatistics statistics_;
    std::chrono::steady_clock::time_point last_stats_update_;

    // ETSI compliance tracking
    std::atomic<uint64_t> compliance_violations_count_;
    std::atomic<uint64_t> thai_encoding_issues_count_;

    // Number of FIBs skipped in the last parseFicData() call due to bad CRC.
    // Atomic: parseFicData() may run on worker threads while the GUI reads
    // the counter from the main thread (documented single-writer, multi-reader).
    std::atomic<uint32_t> fib_crc_failures_count_{0};

    // True once the raw-FIC fallback warning (all FIB CRCs failed) has been
    // logged, so it appears once per parser lifetime instead of per frame.
    bool raw_fic_fallback_warned_{false};

    // User-selected analyser decode options (option-variant matrix rows 4/5);
    // defaults == current behavior.
    AnalyserSettings settings_;

    // Parsing optimization data
    std::unordered_map<uint8_t, uint64_t> fig_type_frequency_;
    std::chrono::steady_clock::time_point performance_optimization_time_;

private:
    // Constants for validation
    static constexpr uint32_t THAI_UNICODE_START = 0x0E00;
    static constexpr uint32_t THAI_UNICODE_END = 0x0E7F;
    static constexpr double TARGET_FPS = 900.0;
    static constexpr double MAX_PARSING_LATENCY_MS = 5.0;
};

/**
 * @brief Factory function for creating optimized FIG parser
 * @param enable_thai_support Enable Thai character support
 * @param strict_compliance Enable strict ETSI compliance
 * @return Unique pointer to configured FIG parser
 */
std::unique_ptr<FigParser> createOptimizedFigParser(
    bool enable_thai_support = true,
    bool strict_compliance = false);

// Type alias for backwards compatibility
using Fig02ServiceComponent = Fig02ServiceInfo::ServiceComponent;

} // namespace eti::fig
