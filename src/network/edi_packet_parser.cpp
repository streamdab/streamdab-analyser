/**
 * @file edi_packet_parser.cpp
 * @brief EDI packet parser implementation
 *
 * @author Backend Developer
 * @date 2025-10-19
 * @copyright Copyright (c) 2025 StreamDAB Analyser Team
 */

#include "edi_packet_parser.hpp"
#include <algorithm>
#include <cstring>
#include <sstream>
#include <iomanip>

namespace edi {

// ============================================================================
// Constants
// ============================================================================

constexpr size_t AF_HEADER_SIZE = 10;      // SYNC(2) + LEN(4) + minimal payload
constexpr size_t TAG_NAME_SIZE = 4;         // TAG name is always 4 bytes
constexpr size_t TAG_HEADER_SIZE = 7;       // NAME(4) + LEN(3)
constexpr uint16_t AF_SYNC_BYTES = constants::AF_SYNC_PATTERN_1;
constexpr size_t AF_MAX_PACKET_SIZE = constants::MAX_AF_PACKET_SIZE;

// ============================================================================
// Helper Functions
// ============================================================================

inline uint16_t read_be16(std::span<const uint8_t> data, size_t offset) {
    return (static_cast<uint16_t>(data[offset]) << 8) |
           static_cast<uint16_t>(data[offset + 1]);
}

inline uint32_t read_be32(std::span<const uint8_t> data, size_t offset) {
    return (static_cast<uint32_t>(data[offset]) << 24) |
           (static_cast<uint32_t>(data[offset + 1]) << 16) |
           (static_cast<uint32_t>(data[offset + 2]) << 8) |
           static_cast<uint32_t>(data[offset + 3]);
}

inline uint32_t read_be24(std::span<const uint8_t> data, size_t offset) {
    return (static_cast<uint32_t>(data[offset]) << 16) |
           (static_cast<uint32_t>(data[offset + 1]) << 8) |
           static_cast<uint32_t>(data[offset + 2]);
}

inline bool hasEnoughData(std::span<const uint8_t> data, size_t required, const char* context) {
    (void)context;  // Unused in release builds
    return data.size() >= required;
}

inline bool isValidTAGName(const std::string& name) {
    if (name.size() != TAG_NAME_SIZE) return false;
    // TAG names should be printable ASCII or '*' wildcard
    for (char c : name) {
        if (!std::isprint(static_cast<unsigned char>(c)) && c != '*') {
            return false;
        }
    }
    return true;
}

// ============================================================================
// CRC-32 Table Initialization
// ============================================================================

const std::array<uint32_t, 256> EDIPacketParser::CRC32_TABLE = EDIPacketParser::initCRC32Table();

std::array<uint32_t, 256> EDIPacketParser::initCRC32Table() {
    std::array<uint32_t, 256> table;
    constexpr uint32_t polynomial = 0xEDB88320; // Reversed CRC-32 polynomial

    for (uint32_t i = 0; i < 256; ++i) {
        uint32_t crc = i;
        for (uint32_t j = 0; j < 8; ++j) {
            if (crc & 1) {
                crc = (crc >> 1) ^ polynomial;
            } else {
                crc >>= 1;
            }
        }
        table[i] = crc;
    }
    return table;
}

// ============================================================================
// Constructor / Destructor
// ============================================================================

EDIPacketParser::EDIPacketParser()
    : m_crc_validation_enabled(true)
    , m_strict_mode(false)
{
    m_statistics.reset();
}

// ============================================================================
// AF Packet Parsing
// ============================================================================

std::optional<AFPacket> EDIPacketParser::parseAFPacket(std::span<const uint8_t> data) {
    // Check minimum AF packet size: SYNC(2) + LEN(4) + CRC(4) = 10 bytes minimum
    if (!hasEnoughData(data, AF_HEADER_SIZE, "AF packet header")) {
        m_statistics.parse_errors++;
        return std::nullopt;
    }

    AFPacket packet;

    // Parse sync bytes (offset 0, 2 bytes)
    packet.sync_pattern = read_be16(data, 0);
    if (packet.sync_pattern != AF_SYNC_BYTES) {
        std::ostringstream oss;
        oss << "Invalid AF sync bytes: expected 0x" << std::hex << std::setw(4) << std::setfill('0') << AF_SYNC_BYTES
            << ", got 0x" << std::setw(4) << std::setfill('0') << packet.sync_pattern;
        reportError(oss.str(), 2);
        m_statistics.af_sync_errors++;
        return std::nullopt;
    }

    // Parse length field (offset 2, 4 bytes) - big endian
    packet.packet_length = read_be32(data, 2);

    // Validate packet length
    if (packet.packet_length > AF_MAX_PACKET_SIZE) {
        std::ostringstream oss;
        oss << "AF packet length exceeds maximum: " << packet.packet_length
            << " > " << AF_MAX_PACKET_SIZE;
        reportError(oss.str(), 2);
        m_statistics.parse_errors++;
        return std::nullopt;
    }

    // Calculate expected total packet size
    size_t expected_size = 2 + 4 + packet.packet_length + 4; // SYNC + LEN + PAYLOAD + CRC
    if (data.size() < expected_size) {
        std::ostringstream oss;
        oss << "AF packet truncated: expected " << expected_size
            << " bytes, got " << data.size();
        reportError(oss.str(), 2);
        m_statistics.parse_errors++;
        return std::nullopt;
    }

    // Extract TAG packet payload (offset 6, length bytes)
    std::span<const uint8_t> tag_data = data.subspan(6, packet.packet_length);

    // Parse CRC (last 4 bytes)
    size_t crc_offset = 6 + packet.packet_length;
    packet.crc = read_be32(data, crc_offset);

    // Validate CRC if enabled - CRC validation always fails on mismatch when enabled
    if (m_crc_validation_enabled) {
        // CRC is calculated over SYNC + LEN + PAYLOAD (excluding CRC itself)
        std::span<const uint8_t> crc_data = data.subspan(0, crc_offset);
        uint32_t calculated_crc = calculateCRC32(crc_data);

        if (calculated_crc != packet.crc) {
            std::ostringstream oss;
            oss << "AF packet CRC mismatch: expected 0x" << std::hex << std::setw(8) << std::setfill('0') << packet.crc
                << ", calculated 0x" << std::setw(8) << std::setfill('0') << calculated_crc;
            reportError(oss.str(), 2); // Error level - CRC failures are always errors
            m_statistics.crc_errors++;
            return std::nullopt; // Always fail on CRC mismatch when validation is enabled
        }
    }

    // Store payload data
    packet.payload.assign(tag_data.begin(), tag_data.end());
    packet.received_time = std::chrono::system_clock::now();

    // Extract TAG items from payload for statistics tracking
    auto tag_items = extractTAGItems(tag_data);

    m_statistics.total_af_packets_received++;
    return packet;
}

// ============================================================================
// TAG Packet Parsing
// ============================================================================

std::optional<TAGPacket> EDIPacketParser::parseTAGPacket(std::span<const uint8_t> data) {
    TAGPacket packet;

    // Extract all TAG items from the data
    auto tag_items = extractTAGItems(data);

    if (tag_items.empty() && !data.empty()) {
        reportError("Failed to extract any TAG items from non-empty data", 1);
        return std::nullopt;
    }

    packet.items = std::move(tag_items);
    m_statistics.tag_packets_parsed++;

    return packet;
}

std::vector<TAGItem> EDIPacketParser::extractTAGItems(std::span<const uint8_t> tag_packet_data) {
    std::vector<TAGItem> items;

    size_t offset = 0;
    while (offset < tag_packet_data.size()) {
        // Check if we have enough data for TAG header
        if (offset + TAG_HEADER_SIZE > tag_packet_data.size()) {
            if (offset < tag_packet_data.size()) {
                // Remaining bytes but not enough for header
                std::ostringstream oss;
                oss << "Incomplete TAG header at offset " << offset
                    << ": " << (tag_packet_data.size() - offset) << " bytes remaining";
                reportError(oss.str(), 1);
            }
            break;
        }

        size_t bytes_consumed = 0;
        auto tag_item = parseTAGItem(tag_packet_data.subspan(offset), bytes_consumed);

        if (!tag_item) {
            // Parse error - stop processing
            reportError("Failed to parse TAG item", 2);
            break;
        }

        // Validate TAG item
        if (!validateTAGItem(*tag_item)) {
            reportError("Invalid TAG item structure: " + tag_item->tag_name, 1);
            if (m_strict_mode) {
                break;
            }
        }

        items.push_back(std::move(*tag_item));
        m_statistics.total_tag_items_parsed++;

        offset += bytes_consumed;
    }

    return items;
}

std::optional<TAGItem> EDIPacketParser::parseTAGItem(std::span<const uint8_t> data,
                                                      size_t& bytes_consumed) {
    bytes_consumed = 0;

    // Check minimum TAG item size
    if (data.size() < TAG_HEADER_SIZE) {
        reportError("TAG item data too small for header", 2);
        return std::nullopt;
    }

    TAGItem item;

    // Parse TAG name (4 bytes as string)
    item.tag_name.assign(reinterpret_cast<const char*>(data.data()), TAG_NAME_SIZE);

    // Parse TAG length (3 bytes, big endian) - note: TAG uses 3-byte length, not 4
    item.tag_length = read_be24(data, TAG_NAME_SIZE);

    // Validate length
    size_t total_item_size = TAG_HEADER_SIZE + item.tag_length;
    if (total_item_size > data.size()) {
        std::ostringstream oss;
        oss << "TAG item length exceeds available data: "
            << item.tag_name << " requires " << total_item_size
            << " bytes, have " << data.size();
        reportError(oss.str(), 2);
        return std::nullopt;
    }

    // Extract TAG value
    item.tag_value.resize(item.tag_length);
    if (item.tag_length > 0) {
        std::memcpy(item.tag_value.data(), data.data() + TAG_HEADER_SIZE, item.tag_length);
    }

    bytes_consumed = total_item_size;
    return item;
}

// ============================================================================
// TAG Item Validation and Search
// ============================================================================

bool EDIPacketParser::validateTAGItem(const TAGItem& item) const {
    // Use built-in validation from TAGItem
    return item.isValid();
}

std::optional<TAGItem> EDIPacketParser::findTAGItem(const TAGPacket& packet,
                                                     const std::string& tag_name) const {
    if (tag_name.size() != constants::MAX_TAG_NAME_LENGTH) {
        return std::nullopt;
    }

    for (const auto& item : packet.items) {
        if (item.isType(tag_name)) {
            return item;
        }
    }

    return std::nullopt;
}

// ============================================================================
// CRC Validation
// ============================================================================

bool EDIPacketParser::validateAFCRC(std::span<const uint8_t> data) const {
    if (data.size() < AF_HEADER_SIZE) {
        return false;
    }

    // Extract stored CRC (last 2 bytes for CRC-16)
    uint16_t stored_crc = read_be16(data, data.size() - 2);

    // Calculate CRC over all data except CRC field
    std::span<const uint8_t> crc_data = data.subspan(0, data.size() - 4);
    uint32_t calculated_crc = calculateCRC32(crc_data);

    return calculated_crc == stored_crc;
}

uint32_t EDIPacketParser::calculateCRC32(std::span<const uint8_t> data) const {
    uint32_t crc = 0xFFFFFFFF; // Initial CRC value

    for (uint8_t byte : data) {
        uint8_t table_index = (crc ^ byte) & 0xFF;
        crc = (crc >> 8) ^ CRC32_TABLE[table_index];
    }

    return crc ^ 0xFFFFFFFF; // Final XOR
}

// ============================================================================
// Error Handling and Statistics
// ============================================================================

void EDIPacketParser::setErrorCallback(ErrorCallback callback) {
    m_error_callback = std::move(callback);
}

const EDIStatistics& EDIPacketParser::getStatistics() const {
    return m_statistics;
}

void EDIPacketParser::resetStatistics() {
    m_statistics.reset();
}

std::string EDIPacketParser::getLastError() const {
    return m_last_error;
}

void EDIPacketParser::setCRCValidation(bool enable) {
    m_crc_validation_enabled = enable;
}

void EDIPacketParser::setStrictMode(bool enable) {
    m_strict_mode = enable;
}

// ============================================================================
// Private Helper Methods
// ============================================================================

uint16_t EDIPacketParser::read_be16(std::span<const uint8_t> data, size_t offset) const {
    return (static_cast<uint16_t>(data[offset]) << 8) |
           static_cast<uint16_t>(data[offset + 1]);
}

uint32_t EDIPacketParser::read_be32(std::span<const uint8_t> data, size_t offset) const {
    return (static_cast<uint32_t>(data[offset]) << 24) |
           (static_cast<uint32_t>(data[offset + 1]) << 16) |
           (static_cast<uint32_t>(data[offset + 2]) << 8) |
           static_cast<uint32_t>(data[offset + 3]);
}

void EDIPacketParser::reportError(const std::string& error, int severity) {
    m_last_error = error;

    if (m_error_callback) {
        m_error_callback(error, severity);
    }
}

bool EDIPacketParser::isValidTAGName(const std::array<char, 4>& name) const {
    // TAG names must be printable ASCII or contain wildcard '*'
    for (char c : name) {
        if (c != '*' && !std::isprint(static_cast<unsigned char>(c))) {
            return false;
        }
    }
    return true;
}

bool EDIPacketParser::hasEnoughData(std::span<const uint8_t> data,
                                    size_t required,
                                    const std::string& context) const {
    if (data.size() < required) {
        std::ostringstream oss;
        oss << "Insufficient data for " << context << ": need " << required
            << " bytes, have " << data.size();
        const_cast<EDIPacketParser*>(this)->reportError(oss.str(), 2);
        return false;
    }
    return true;
}

} // namespace edi
