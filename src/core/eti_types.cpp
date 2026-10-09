/**
 * @file eti_types.cpp
 * @brief ETSI-Compliant ETI data type implementations
 *
 * Implementation of the fundamental data types, structures, and methods
 * used throughout the ETI Stream Analyser for DAB/DAB+ processing.
 *
 * @author Standards Compliance Agent
 * @date 2025
 * @copyright Copyright (c) 2025 ETI Stream Analyser Team
 */

#include "eti_types.hpp"
#include "crc16.hpp"
#include <algorithm>
#include <cstring>
#include <numeric>

namespace eti {

//==============================================================================
// ETI Format Detection Implementation
//==============================================================================

/**
 * @brief Detect ETI format from frame data
 *
 * Analyzes the sync pattern to determine the ETI format variant:
 * - ETI-LI: 0x49 0x93 0x1E 0x03 (Linear, most common)
 * - ETI-NI: 0xFF 0xF8 or 0xFF 0x07 (Network Independent, alternative)
 *
 * Reference: ETSI EN 300 799 Section 5.1
 *
 * @param data Pointer to frame data (at least 4 bytes required)
 * @param size Size of data buffer
 * @return ETIFormat Detected format type (ETI_LI, ETI_NI, or UNKNOWN)
 */
ETIFormat detectETIFormat(const uint8_t* data, size_t size) {
    // Validate minimum buffer size
    if (data == nullptr || size < 4) {
        return ETIFormat::UNKNOWN;
    }

    // Check ETI-LI sync pattern (0x49 0x93 0x1E 0x03)
    if (data[0] == 0x49 && data[1] == 0x93 &&
        data[2] == 0x1E && data[3] == 0x03) {
        return ETIFormat::ETI_LI;
    }

    // Check ETI-NI sync pattern variants
    // Variant 1: 0xFF 0xF8 (most common ETI-NI)
    // Variant 2: 0xFF 0x07 (alternative ETI-NI)
    if (data[0] == 0xFF && (data[1] == 0xF8 || data[1] == 0x07)) {
        return ETIFormat::ETI_NI;
    }

    // No recognized sync pattern
    return ETIFormat::UNKNOWN;
}

/**
 * @brief Convert ETI format enum to string representation
 *
 * @param format ETI format enum value
 * @return const char* Human-readable format name
 */
const char* formatToString(ETIFormat format) {
    switch (format) {
        case ETIFormat::ETI_LI:
            return "ETI-LI";
        case ETIFormat::ETI_NI:
            return "ETI-NI";
        case ETIFormat::ETI_NA:
            return "ETI-NA";
        case ETIFormat::UNKNOWN:
        default:
            return "UNKNOWN";
    }
}

//==============================================================================
// EtiFicField Implementation
//==============================================================================

std::vector<FigBlock> EtiFicField::decode_fig_blocks() const {
    // Byte-identical to the historical strict FIB walk (no env override).
    // decodeFigBlocks() itself is header-inline (see eti_types.hpp).
    return decodeFigBlocks(FicDecodeMode::Strict).fig_blocks;
}

bool EtiFicField::validate_fic_crc() const {
    const bool fib_structured = is_fib_structured();

    if (!fib_structured) {
        // Legacy raw 32-byte FIG area: no FIB CRC to validate — check for data
        return std::any_of(fic_data.begin(), fic_data.begin() + fic_size,
                           [](uint8_t byte) { return byte != 0; });
    }

    // FIB-structured FIC: every FIB must pass its CRC
    const size_t fib_count_effective = fibi_count();
    size_t valid_fibs = 0;
    for (size_t fib = 0; fib < fib_count_effective; ++fib) {
        const size_t fib_base = fib * ETI_FIC_FIB_SIZE;
        if (fib_base + ETI_FIC_FIB_SIZE > fic_size) {
            break;
        }
        const uint8_t* fib_data = fic_data.data() + fib_base;
        const uint16_t stored_crc = static_cast<uint16_t>((fib_data[30] << 8) | fib_data[31]);
        if (fib_crc_expected(fib_data) == stored_crc) {
            ++valid_fibs;
        }
    }

    return valid_fibs == fib_count_effective && valid_fibs > 0;
}

bool EtiFicField::contains_fig_type(uint8_t fig_type) const {
    auto fig_blocks = decode_fig_blocks();
    return std::any_of(fig_blocks.begin(), fig_blocks.end(),
                      [fig_type](const FigBlock& fig) {
                          return fig.fig_type == fig_type;
                      });
}

//==============================================================================
// EtiMscField Implementation
//==============================================================================

std::vector<uint8_t> EtiMscField::extract_subchannel_data(const SubChannelInfo& subchannel) const {
    std::vector<uint8_t> subchannel_data;

    // Validate subchannel boundaries
    if (!subchannel.validate_boundaries()) {
        return subchannel_data; // Return empty vector for invalid subchannel
    }

    // Calculate byte positions
    size_t start_byte = subchannel.start_address * CAPACITY_UNIT_SIZE;
    size_t size_bytes = subchannel.size * CAPACITY_UNIT_SIZE;

    // Ensure we don't read beyond MSC data
    if (start_byte + size_bytes > ETI_MSC_SIZE) {
        size_bytes = ETI_MSC_SIZE - start_byte;
    }

    // Extract subchannel data
    if (size_bytes > 0) {
        subchannel_data.assign(msc_data.data() + start_byte, msc_data.data() + start_byte + size_bytes);
    }

    return subchannel_data;
}

bool EtiMscField::validate_subchannel_organization(const std::vector<SubChannelInfo>& subchannels) const {
    if (subchannels.empty()) {
        return true; // No subchannels is valid
    }

    // Check for overlapping subchannels
    for (size_t i = 0; i < subchannels.size(); ++i) {
        for (size_t j = i + 1; j < subchannels.size(); ++j) {
            const auto& a = subchannels[i];
            const auto& b = subchannels[j];

            // Check for overlap
            if (!(a.get_end_address() < b.start_address ||
                  b.get_end_address() < a.start_address)) {
                return false; // Overlap detected
            }
        }
    }

    // Check total capacity doesn't exceed MSC capacity
    uint16_t total_capacity = std::accumulate(subchannels.begin(), subchannels.end(), 0,
                                            [](uint16_t sum, const SubChannelInfo& sc) {
                                                return sum + sc.size;
                                            });

    return total_capacity <= MAX_CAPACITY_UNITS;
}

//==============================================================================
// EtiCrcField Implementation
//==============================================================================

bool EtiCrcField::validate_frame(const EtiFrame& frame) const {
    // Calculate CRC for the frame (excluding the CRC field itself)
    uint32_t calculated_crc = calculate_frame_crc(frame);
    uint32_t frame_crc = get_crc_value();

    return calculated_crc == frame_crc;
}

void EtiCrcField::calculate_crc(const EtiFrame& frame) {
    uint32_t crc = calculate_frame_crc(frame);

    // Store CRC in big-endian format
    crc_bytes[0] = static_cast<uint8_t>((crc >> 24) & 0xFF);
    crc_bytes[1] = static_cast<uint8_t>((crc >> 16) & 0xFF);
    crc_bytes[2] = static_cast<uint8_t>((crc >> 8) & 0xFF);
    crc_bytes[3] = static_cast<uint8_t>(crc & 0xFF);
}

uint32_t EtiCrcField::calculate_frame_crc(const EtiFrame& frame) const {
    // CRC-32 calculation for ETI frame (excluding CRC field)
    const uint8_t* data = frame.data();
    size_t length = frame.size() - ETI_CRC_SIZE;

    uint32_t crc = 0xFFFFFFFF;
    const uint32_t polynomial = 0x04C11DB7;

    for (size_t i = 0; i < length; ++i) {
        crc ^= static_cast<uint32_t>(data[i]) << 24;

        for (int j = 0; j < 8; ++j) {
            if (crc & 0x80000000) {
                crc = (crc << 1) ^ polynomial;
            } else {
                crc <<= 1;
            }
        }
    }

    return crc ^ 0xFFFFFFFF;
}

//==============================================================================
// EtiFrame Implementation
//==============================================================================

std::vector<SubChannelInfo> EtiFrame::get_subchannel_info() const {
    std::vector<SubChannelInfo> subchannels;

    // Decode FIC to extract subchannel organization (FIG 0/1)
    auto fic = get_fic_field();
    auto fig_blocks = fic.decode_fig_blocks();

    for (const auto& fig : fig_blocks) {
        if (fig.fig_type == 0 && fig.get_extension() == 1) {
            // FIG 0/1: Sub-channel organization
            parse_fig_0_1(fig, subchannels);
        }
    }

    return subchannels;
}

Ensemble EtiFrame::extract_ensemble_info() const {
    Ensemble ensemble;

    // Initialize with default values
    ensemble.ensemble_id = 0;
    ensemble.country_id = 0;
    ensemble.extended_country_code = 0;
    ensemble.cif_count = 0;
    ensemble.occurrence_change = 0;
    ensemble.alarm_flag = false;

    // Decode FIC to extract ensemble information
    auto fic = get_fic_field();
    auto fig_blocks = fic.decode_fig_blocks();

    for (const auto& fig : fig_blocks) {
        switch (fig.fig_type) {
            case 0:
                switch (fig.get_extension()) {
                    case 0: // Ensemble information
                        parse_fig_0_0(fig, ensemble);
                        break;
                    case 1: // Sub-channel organization
                        parse_fig_0_1(fig, ensemble.sub_channels);
                        break;
                    case 2: // Service organization
                        parse_fig_0_2(fig, ensemble);
                        break;
                    case 3: // Service component definition
                        parse_fig_0_3(fig, ensemble);
                        break;
                }
                break;

            case 1:
                switch (fig.get_extension()) {
                    case 0: // Ensemble label
                        parse_fig_1_0(fig, ensemble);
                        break;
                    case 1: // Programme service label
                        parse_fig_1_1(fig, ensemble);
                        break;
                }
                break;
        }
    }

    return ensemble;
}

void EtiFrame::parse_fig_0_0(const FigBlock& fig, Ensemble& ensemble) const {
    if (fig.data.size() < 4) {
        return; // Invalid FIG 0/0
    }

    // Extract ensemble information from FIG 0/0
    ensemble.ensemble_id = (static_cast<uint16_t>(fig.data[1]) << 8) | fig.data[2];

    if (fig.data.size() >= 6) {
        uint8_t change_flags = fig.data[3];
        ensemble.occurrence_change = (change_flags >> 6) & 0x03;
        ensemble.alarm_flag = (change_flags & 0x20) != 0;

        ensemble.cif_count = ((static_cast<uint16_t>(fig.data[4]) << 5) |
                             ((fig.data[5] & 0xF8) >> 3)) & 0x1FFF;
    }
}

void EtiFrame::parse_fig_0_1(const FigBlock& fig, std::vector<SubChannelInfo>& subchannels) const {
    if (fig.data.size() < 3) {
        return; // Invalid FIG 0/1
    }

    size_t offset = 1; // Skip extension field

    while (offset + 2 < fig.data.size()) {
        SubChannelInfo subchannel;

        subchannel.sub_channel_id = fig.data[offset] & 0x3F;
        subchannel.start_address = ((static_cast<uint16_t>(fig.data[offset + 1]) << 2) |
                                   ((fig.data[offset + 2] & 0xC0) >> 6)) & 0x3FF;

        // Check for long or short form
        if (fig.data[offset + 2] & 0x20) {
            // Long form (UEP)
            if (offset + 3 < fig.data.size()) {
                subchannel.uep_flag = true;
                subchannel.protection_level = (fig.data[offset + 2] & 0x18) >> 3;
                subchannel.size = ((static_cast<uint16_t>(fig.data[offset + 2] & 0x07) << 7) |
                                  ((fig.data[offset + 3] & 0xFE) >> 1)) & 0x3FF;
                offset += 4;
            } else {
                break;
            }
        } else {
            // Short form (EEP)
            subchannel.uep_flag = false;
            subchannel.size = ((static_cast<uint16_t>(fig.data[offset + 2] & 0x1F) << 5) |
                              ((fig.data[offset + 3] & 0xF8) >> 3)) & 0x3FF;
            subchannel.protection_level = fig.data[offset + 3] & 0x07;
            offset += 4;
        }

        // Validate and add subchannel
        if (subchannel.validate_boundaries() && subchannel.validate_sub_channel_id()) {
            subchannels.push_back(subchannel);
        }
    }
}

void EtiFrame::parse_fig_0_2(const FigBlock& fig, Ensemble& ensemble) const {
    if (fig.data.size() < 3) {
        return; // Invalid FIG 0/2
    }

    size_t offset = 1; // Skip extension field

    while (offset + 2 < fig.data.size()) {
        DabService service;

        service.service_id = (static_cast<uint16_t>(fig.data[offset]) << 8) | fig.data[offset + 1];

        if (offset + 2 < fig.data.size()) {
            uint8_t flags = fig.data[offset + 2];
            service.country_id = (flags & 0xF0) >> 4;
            service.extended_country_code = flags & 0x0F;
            service.is_programme = true; // Assume programme service for FIG 0/2
        }

        offset += 3;

        // Add service to ensemble
        if (service.validate_service_id()) {
            ensemble.services.push_back(service);
        }
    }
}

void EtiFrame::parse_fig_0_3(const FigBlock& fig, Ensemble& ensemble) const {
    if (fig.data.size() < 5) {
        return; // Invalid FIG 0/3
    }

    size_t offset = 1; // Skip extension field

    while (offset + 4 < fig.data.size()) {
        uint16_t service_id = (static_cast<uint16_t>(fig.data[offset]) << 8) | fig.data[offset + 1];

        // Find corresponding service
        auto service_it = std::find_if(ensemble.services.begin(), ensemble.services.end(),
                                      [service_id](const DabService& svc) {
                                          return svc.service_id == service_id;
                                      });

        if (service_it != ensemble.services.end()) {
            ServiceComponent component;
            component.service_id = service_id;
            component.component_type = fig.data[offset + 2] & 0x3F;
            component.sub_channel_id = fig.data[offset + 3] & 0x3F;
            component.tmid = (fig.data[offset + 4] & 0xC0) >> 6;
            component.asc_ty = fig.data[offset + 4] & 0x3F;
            component.primary = (fig.data[offset + 2] & 0x80) != 0;
            component.ca_flag = (fig.data[offset + 2] & 0x40) != 0;

            service_it->components.push_back(component);
        }

        offset += 5;
    }
}

void EtiFrame::parse_fig_1_0(const FigBlock& fig, Ensemble& ensemble) const {
    if (fig.data.size() < 18) {
        return; // Invalid FIG 1/0
    }

    uint16_t fig_ensemble_id = (static_cast<uint16_t>(fig.data[1]) << 8) | fig.data[2];

    if (fig_ensemble_id == ensemble.ensemble_id) {
        // Extract ensemble label (16 characters)
        ensemble.label.assign(reinterpret_cast<const char*>(fig.data.data() + 3), 16);

        // Remove trailing spaces and null characters
        ensemble.label.erase(ensemble.label.find_last_not_of(" \0") + 1);

        if (fig.data.size() >= 20) {
            ensemble.character_flag = (static_cast<uint16_t>(fig.data[19]) << 8) | fig.data[20];
        }
    }
}

void EtiFrame::parse_fig_1_1(const FigBlock& fig, Ensemble& ensemble) const {
    if (fig.data.size() < 18) {
        return; // Invalid FIG 1/1
    }

    uint16_t service_id = (static_cast<uint16_t>(fig.data[1]) << 8) | fig.data[2];

    // Find corresponding service
    auto service_it = std::find_if(ensemble.services.begin(), ensemble.services.end(),
                                  [service_id](const DabService& svc) {
                                      return svc.service_id == service_id;
                                  });

    if (service_it != ensemble.services.end()) {
        // Extract service label (16 characters)
        service_it->label.assign(reinterpret_cast<const char*>(fig.data.data() + 3), 16);

        // Remove trailing spaces and null characters
        service_it->label.erase(service_it->label.find_last_not_of(" \0") + 1);

        if (fig.data.size() >= 20) {
            service_it->character_flag = (static_cast<uint16_t>(fig.data[19]) << 8) | fig.data[20];
        }
    }
}

} // namespace eti
