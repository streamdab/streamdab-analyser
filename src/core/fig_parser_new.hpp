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
#include <QObject>
#include <QString>
#include <QByteArray>
#include <memory>
#include <vector>
#include <unordered_map>
#include <chrono>
#include <atomic>

namespace eti::fig {

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
