// SPDX-License-Identifier: MIT
// Copyright (c) 2025 StreamDAB Analyser Project
//
// EDI Protocol Type Definitions (ETSI TS 102 693)
//
// This file contains type definitions for the EDI (Encapsulation of DAB Interfaces)
// protocol as specified in ETSI TS 102 693. EDI provides a standard way to transport
// ETI data over IP networks using either AF (Application Format) or PFT
// (Protection/Fragmentation/Transport) packets.
//
// References:
// - ETSI TS 102 693 V1.2.1 (2016-12) - DAB EDI Transport Protocol
// - AF Packets: Simple framing without FEC or fragmentation
// - PFT Packets: Advanced framing with Reed-Solomon FEC and fragmentation support
//
// Usage Example:
//   edi::AFPacket af_packet;
//   if (edi::parseAFPacket(data, af_packet)) {
//       auto tags = edi::extractTAGItems(af_packet.payload);
//       for (const auto& tag : tags.items) {
//           if (tag.tag_name == "deti") {
//               // Process DAB ETI TAG
//           }
//       }
//   }

#pragma once

#include <array>
#include <algorithm>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <vector>
#include <span>
#include <chrono>

namespace edi {

// ============================================================================
// EDI Protocol Constants (TS 102 693 Section 5)
// ============================================================================

namespace constants {
    // AF Packet sync patterns (Section 5.1) - Big-endian 'AF' = 0x41 0x46
    constexpr uint16_t AF_SYNC_PATTERN_1 = 0x4146;  // 'AF' = 0x41 0x46 (primary)
    constexpr uint16_t AF_SYNC_PATTERN_2 = 0xAF01;  // Alternative pattern

    // PFT Packet sync patterns (Section 6)
    constexpr uint16_t PFT_SYNC_PATTERN_1 = 0x5046;  // 'PF' protocol sync
    constexpr uint16_t PFT_SYNC_PATTERN_2 = 0x5054;  // 'PT' protocol sync

    // Protocol versions
    constexpr uint8_t AF_PROTOCOL_VERSION = 0x01;
    constexpr uint8_t PFT_PROTOCOL_VERSION = 0x01;

    // Maximum packet sizes
    constexpr size_t MAX_AF_PACKET_SIZE = 8192;      // Typical maximum
    constexpr size_t MAX_PFT_PACKET_SIZE = 65535;    // Maximum UDP payload
    constexpr size_t MAX_TAG_NAME_LENGTH = 4;        // TAG names are 4 chars
    constexpr size_t MAX_TAG_PAYLOAD_SIZE = 8191;    // Maximum TAG payload

    // FEC types (Section 6.3.1)
    constexpr uint8_t FEC_TYPE_NONE = 0x00;
    constexpr uint8_t FEC_TYPE_REED_SOLOMON = 0x01;  // Reed-Solomon FEC

    // Common TAG names (Section 5.3)
    constexpr std::string_view TAG_DETI = "deti";    // DAB ETI data
    constexpr std::string_view TAG_EST = "est*";     // ETI STream information
    constexpr std::string_view TAG_PTR = "*ptr";     // PoinTeR to data
    constexpr std::string_view TAG_DL = "dl**";      // Data Length
    constexpr std::string_view TAG_SSN = "ss**";     // Sequence Number
    constexpr std::string_view TAG_TC = "tc**";      // Time Code
    constexpr std::string_view TAG_ODR = "odr*";     // ODR-specific extensions

} // namespace constants

// ============================================================================
// Forward Declarations
// ============================================================================

struct AFPacket;
struct PFTPacket;
struct TAGItem;
struct TAGPacket;
struct EDIPacket;

// ============================================================================
// TAG Item Structure (TS 102 693 Section 5.3)
// ============================================================================

/**
 * TAG Item - Basic unit of EDI data encapsulation
 *
 * Structure (variable length):
 *   Name:   4 bytes (ASCII, padded with '*')
 *   Length: 3 bytes (MSB first, max 8191 bytes)
 *   Value:  Length bytes
 *
 * Common TAG types:
 *   - "deti": Contains complete ETI frame or ETI data
 *   - "est*": ETI stream information and configuration
 *   - "*ptr": Pointer to data in another TAG
 *   - "dl**": Data length indicator
 *   - "ss**": Sequence number for ordering
 *   - "tc**": Time code for synchronization
 */
struct TAGItem {
    std::string tag_name;              // 4-character TAG name (e.g., "deti", "est*")
    uint32_t tag_length{0};            // Length of tag_value (max 8191 bytes)
    std::vector<uint8_t> tag_value;    // TAG payload data

    // Move-only semantics for efficient data handling
    TAGItem() = default;
    TAGItem(const TAGItem&) = default;
    TAGItem(TAGItem&&) noexcept = default;
    TAGItem& operator=(const TAGItem&) = default;
    TAGItem& operator=(TAGItem&&) noexcept = default;

    /**
     * Validate TAG item structure
     * @return true if TAG is valid according to TS 102 693
     */
    [[nodiscard]] constexpr bool isValid() const noexcept {
        return tag_name.size() == constants::MAX_TAG_NAME_LENGTH &&
               tag_length <= constants::MAX_TAG_PAYLOAD_SIZE &&
               tag_value.size() == tag_length;
    }

    /**
     * Check if this is a specific TAG type
     * @param name TAG name to compare (e.g., "deti")
     * @return true if TAG name matches
     */
    [[nodiscard]] constexpr bool isType(std::string_view name) const noexcept {
        return tag_name == name;
    }

    /**
     * Get TAG value as span for zero-copy access
     * @return Span view of TAG value data
     */
    [[nodiscard]] constexpr std::span<const uint8_t> valueSpan() const noexcept {
        return std::span<const uint8_t>(tag_value);
    }
};

// ============================================================================
// TAG Packet Structure (TS 102 693 Section 5.3)
// ============================================================================

/**
 * TAG Packet - Collection of TAG items
 *
 * A TAG packet contains one or more TAG items. Typically extracted from
 * AF or PFT packet payloads after deframing.
 *
 * Example structure:
 *   [TAG Item 1: "deti" with ETI frame data]
 *   [TAG Item 2: "ss**" with sequence number]
 *   [TAG Item 3: "tc**" with timestamp]
 */
struct TAGPacket {
    std::vector<TAGItem> items;        // Ordered list of TAG items

    // Move-only semantics for efficient data handling
    TAGPacket() = default;
    TAGPacket(const TAGPacket&) = default;
    TAGPacket(TAGPacket&&) noexcept = default;
    TAGPacket& operator=(const TAGPacket&) = default;
    TAGPacket& operator=(TAGPacket&&) noexcept = default;

    /**
     * Find first TAG item with specified name
     * @param name TAG name to search for (e.g., "deti")
     * @return Pointer to TAG item if found, nullptr otherwise
     */
    [[nodiscard]] const TAGItem* findTag(std::string_view name) const noexcept {
        for (const auto& item : items) {
            if (item.tag_name == name) {
                return &item;
            }
        }
        return nullptr;
    }

    /**
     * Get all TAG items with specified name
     * @param name TAG name to search for
     * @return Vector of pointers to matching TAG items
     */
    [[nodiscard]] std::vector<const TAGItem*> findAllTags(std::string_view name) const {
        std::vector<const TAGItem*> result;
        for (const auto& item : items) {
            if (item.tag_name == name) {
                result.push_back(&item);
            }
        }
        return result;
    }

    /**
     * Validate all TAG items in packet
     * @return true if all TAGs are valid
     */
    [[nodiscard]] bool isValid() const noexcept {
        return !items.empty() &&
               std::all_of(items.begin(), items.end(),
                          [](const auto& item) { return item.isValid(); });
    }

    /**
     * Get total size of all TAG items
     * @return Total bytes used by all TAGs (including headers)
     */
    [[nodiscard]] size_t totalSize() const noexcept {
        size_t total = 0;
        for (const auto& item : items) {
            total += 7 + item.tag_length;  // 4-byte name + 3-byte length + payload
        }
        return total;
    }
};

// ============================================================================
// AF (Application Format) Packet Structure (TS 102 693 Section 5)
// ============================================================================

/**
 * AF Packet - Simple EDI packet format without FEC or fragmentation
 *
 * Structure:
 *   Offset | Size | Field
 *   -------|------|-------
 *   0      | 2    | Sync pattern (0xAF01)
 *   2      | 4    | Packet length (big-endian)
 *   6      | 2    | Sequence number (optional, big-endian)
 *   8      | 1    | Protocol version (0x01)
 *   9      | 1    | Protocol type (0x01 for TAG packet)
 *   10     | N    | Payload (TAG items)
 *   10+N   | 4    | CRC-32 (IEEE 802.3, polynomial 0xEDB88320)
 *
 * AF packets provide simple framing suitable for reliable networks.
 * No FEC or fragmentation support - entire ETI frame must fit in one packet.
 */
struct AFPacket {
    uint16_t sync_pattern{constants::AF_SYNC_PATTERN_1};  // Sync bytes (0xAF01)
    uint32_t packet_length{0};                             // Total packet length
    uint16_t sequence_number{0};                           // Sequence counter
    uint8_t protocol_version{constants::AF_PROTOCOL_VERSION};  // Protocol version
    uint8_t protocol_type{0x01};                           // Type (0x01 = TAG packet)
    std::vector<uint8_t> payload;                          // TAG packet data
    uint32_t crc{0};                                       // CRC-32 checksum

    // Metadata
    std::chrono::system_clock::time_point received_time;   // Reception timestamp

    // Move-only semantics
    AFPacket() = default;
    AFPacket(const AFPacket&) = default;
    AFPacket(AFPacket&&) noexcept = default;
    AFPacket& operator=(const AFPacket&) = default;
    AFPacket& operator=(AFPacket&&) noexcept = default;

    /**
     * Validate AF packet structure
     * @return true if packet structure is valid
     */
    [[nodiscard]] constexpr bool isValid() const noexcept {
        return (sync_pattern == constants::AF_SYNC_PATTERN_1 ||
                sync_pattern == constants::AF_SYNC_PATTERN_2) &&
               protocol_version == constants::AF_PROTOCOL_VERSION &&
               packet_length <= constants::MAX_AF_PACKET_SIZE &&
               !payload.empty();
    }

    /**
     * Validate CRC checksum
     * @return true if CRC is correct
     */
    [[nodiscard]] bool validateCRC() const noexcept;

    /**
     * Calculate CRC-32 for packet
     * @return Calculated CRC-32 value
     */
    [[nodiscard]] uint32_t calculateCRC() const noexcept;
    /**
     * Get payload as span for zero-copy access
     * @return Span view of payload data
     */
    [[nodiscard]] constexpr std::span<const uint8_t> payloadSpan() const noexcept {
        return std::span<const uint8_t>(payload);
    }
};

// ============================================================================
// PFT (Protection/Fragmentation/Transport) Packet (TS 102 693 Section 6)
// ============================================================================

/**
 * PFT Packet - Advanced EDI packet with FEC and fragmentation support
 *
 * Structure:
 *   Offset | Size | Field
 *   -------|------|-------
 *   0      | 2    | Sync pattern (0x5046)
 *   2      | 2    | Packet length (big-endian)
 *   4      | 1    | Protocol version
 *   5      | 3    | Pseq (PFT sequence number, 24-bit)
 *   8      | 3    | Findex (fragment index, 24-bit)
 *   11     | 3    | Fcount (fragment count, 24-bit)
 *   14     | 1    | FEC type (0x00=None, 0x01=Reed-Solomon)
 *   15     | 1    | Address flag + Plen
 *   16     | N    | Optional source/destination addresses
 *   16+N   | M    | Payload (fragment data)
 *   16+N+M | 2    | CRC-16
 *
 * PFT provides fragmentation (splitting large ETI frames across packets)
 * and Reed-Solomon FEC for error correction on unreliable networks.
 */
struct PFTPacket {
    uint16_t sync_pattern{constants::PFT_SYNC_PATTERN_1};  // Sync bytes (0x5046)
    uint16_t packet_length{0};                              // Total packet length
    uint8_t protocol_version{constants::PFT_PROTOCOL_VERSION};  // Protocol version
    uint32_t pseq{0};                   // PFT sequence number (24-bit, identifies AF packet)
    uint32_t findex{0};                 // Fragment index (24-bit, 0-based)
    uint32_t fcount{0};                 // Fragment count (24-bit, total fragments)
    uint8_t fec_type{constants::FEC_TYPE_NONE};  // FEC algorithm type
    uint8_t address_flag{0};            // Address present flag
    uint16_t plen{0};                   // Payload length

    // Optional addressing (when address_flag is set)
    std::optional<uint32_t> source_address;       // Source IP/identifier
    std::optional<uint32_t> destination_address;  // Destination IP/identifier

    std::vector<uint8_t> payload;       // Fragment payload data
    uint16_t crc{0};                    // CRC-16 checksum

    // Reed-Solomon FEC parameters (when fec_type != 0x00)
    std::optional<uint8_t> rs_k;        // RS data symbols
    std::optional<uint8_t> rs_z;        // RS parity symbols

    // Metadata
    std::chrono::system_clock::time_point received_time;  // Reception timestamp

    // Move-only semantics
    PFTPacket() = default;
    PFTPacket(const PFTPacket&) = default;
    PFTPacket(PFTPacket&&) noexcept = default;
    PFTPacket& operator=(const PFTPacket&) = default;
    PFTPacket& operator=(PFTPacket&&) noexcept = default;

    /**
     * Validate PFT packet structure
     * @return true if packet structure is valid
     */
    [[nodiscard]] constexpr bool isValid() const noexcept {
        return (sync_pattern == constants::PFT_SYNC_PATTERN_1 ||
                sync_pattern == constants::PFT_SYNC_PATTERN_2) &&
               protocol_version == constants::PFT_PROTOCOL_VERSION &&
               packet_length <= constants::MAX_PFT_PACKET_SIZE &&
               findex < fcount &&
               fcount > 0 &&
               true;
    }

    /**
     * Check if this is the first fragment
     * @return true if findex == 0
     */
    [[nodiscard]] constexpr bool isFirstFragment() const noexcept {
        return findex == 0;
    }

    /**
     * Check if this is the last fragment
     * @return true if findex == fcount - 1
     */
    [[nodiscard]] constexpr bool isLastFragment() const noexcept {
        return findex + 1 == fcount;
    }

    /**
     * Check if this packet uses FEC
     * @return true if FEC is enabled
     */
    [[nodiscard]] constexpr bool hasFEC() const noexcept {
        return fec_type != constants::FEC_TYPE_NONE;
    }

    /**
     * Validate CRC checksum
     * @return true if CRC is correct
     */
    [[nodiscard]] bool validateCRC() const noexcept;

    /**
     * Calculate CRC-16 for packet
     * @return Calculated CRC-16 value
     */
    [[nodiscard]] uint16_t calculateCRC() const noexcept;

    /**
     * Get payload as span for zero-copy access
     * @return Span view of payload data
     */
    [[nodiscard]] constexpr std::span<const uint8_t> payloadSpan() const noexcept {
        return std::span<const uint8_t>(payload);
    }
};

// ============================================================================
// EDI Packet Variant (Generic EDI packet container)
// ============================================================================

/**
 * EDI Packet Type Enumeration
 */
enum class EDIPacketType : uint8_t {
    Unknown = 0,
    AF = 1,      // Application Format packet
    PFT = 2,     // Protection/Fragmentation/Transport packet
    TAG = 3      // Extracted TAG packet
};

/**
 * Generic EDI Packet Container
 *
 * Provides unified interface for handling both AF and PFT packets.
 * Use this when packet type is determined at runtime.
 */
struct EDIPacket {
    EDIPacketType type{EDIPacketType::Unknown};
    std::optional<AFPacket> af_packet;
    std::optional<PFTPacket> pft_packet;
    std::optional<TAGPacket> tag_packet;

    // Metadata
    std::chrono::system_clock::time_point received_time;
    size_t raw_size{0};

    /**
     * Check if packet is valid
     * @return true if packet contains valid data
     */
    [[nodiscard]] bool isValid() const noexcept {
        switch (type) {
            case EDIPacketType::AF:
                return af_packet.has_value() && af_packet->isValid();
            case EDIPacketType::PFT:
                return pft_packet.has_value() && pft_packet->isValid();
            case EDIPacketType::TAG:
                return tag_packet.has_value() && tag_packet->isValid();
            default:
                return false;
        }
    }

    /**
     * Get packet type as string
     * @return Human-readable packet type name
     */
    [[nodiscard]] constexpr std::string_view typeString() const noexcept {
        switch (type) {
            case EDIPacketType::AF:  return "AF";
            case EDIPacketType::PFT: return "PFT";
            case EDIPacketType::TAG: return "TAG";
            default: return "Unknown";
        }
    }
};

// ============================================================================
// PFT Fragment Reassembly Context (TS 102 693 Section 6.4)
// ============================================================================

/**
 * PFT Fragment Reassembly State
 *
 * Tracks reassembly of fragmented AF packets from PFT fragments.
 * Maintains fragment buffers and handles Reed-Solomon FEC decoding.
 */
struct PFTReassemblyContext {
    uint32_t pseq{0};                              // PFT sequence number
    uint32_t expected_fcount{0};                   // Expected fragment count
    std::vector<bool> received_fragments;          // Fragment reception bitmap
    std::vector<std::vector<uint8_t>> fragments;   // Fragment payloads
    uint8_t fec_type{constants::FEC_TYPE_NONE};    // FEC algorithm

    // Reed-Solomon FEC context
    std::optional<uint8_t> rs_k;                   // RS data symbols
    std::optional<uint8_t> rs_z;                   // RS parity symbols

    // Timing and state
    std::chrono::system_clock::time_point first_fragment_time;
    std::chrono::system_clock::time_point last_fragment_time;
    size_t received_count{0};

    /**
     * Add fragment to reassembly buffer
     * @param packet PFT packet to add
     * @return true if fragment was added successfully
     */
    bool addFragment(const PFTPacket& packet);

    /**
     * Check if all fragments received
     * @return true if reassembly is complete
     */
    [[nodiscard]] bool isComplete() const noexcept {
        return received_count == expected_fcount;
    }

    /**
     * Check if FEC can recover missing fragments
     * @return true if FEC recovery is possible
     */
    [[nodiscard]] bool canRecover() const noexcept;

    /**
     * Reassemble AF packet from fragments
     * @return Reassembled AF packet if successful
     */
    [[nodiscard]] std::optional<AFPacket> reassemble() const;

    /**
     * Apply Reed-Solomon FEC recovery
     * @return true if FEC recovery succeeded
     */
    bool applyFEC();

    /**
     * Get reassembly progress percentage
     * @return Progress as percentage (0-100)
     */
    [[nodiscard]] constexpr double progress() const noexcept {
        return expected_fcount > 0
            ? (100.0 * received_count) / expected_fcount
            : 0.0;
    }
};

// ============================================================================
// Helper Functions (Parsing and Validation)
// ============================================================================

/**
 * Parse TAG name from raw bytes
 * @param data Pointer to 4-byte TAG name
 * @return TAG name as string (4 characters)
 */
[[nodiscard]] std::string parseTAGName(const uint8_t* data) noexcept;

/**
 * Parse TAG length field (24-bit big-endian)
 * @param data Pointer to 3-byte length field
 * @return TAG length value
 */
[[nodiscard]] constexpr uint32_t parseTAGLength(const uint8_t* data) noexcept {
    return (static_cast<uint32_t>(data[0]) << 16) |
           (static_cast<uint32_t>(data[1]) << 8) |
           static_cast<uint32_t>(data[2]);
}

/**
 * Encode TAG length to bytes (24-bit big-endian)
 * @param length Length value to encode
 * @param output Pointer to 3-byte output buffer
 */
constexpr void encodeTAGLength(uint32_t length, uint8_t* output) noexcept {
    output[0] = static_cast<uint8_t>((length >> 16) & 0xFF);
    output[1] = static_cast<uint8_t>((length >> 8) & 0xFF);
    output[2] = static_cast<uint8_t>(length & 0xFF);
}

/**
 * Calculate CRC-16 checksum (CCITT polynomial 0x1021)
 * @param data Data to checksum
 * @param length Data length in bytes
 * @return CRC-16 value
 */
[[nodiscard]] uint16_t calculateCRC16(const uint8_t* data, size_t length) noexcept;

/**
 * Calculate CRC-16 checksum for span
 * @param data Data span to checksum
 * @return CRC-16 value
 */
[[nodiscard]] inline uint16_t calculateCRC16(std::span<const uint8_t> data) noexcept {
    return calculateCRC16(data.data(), data.size());
}

/**
 * Validate TAG name format
 * @param name TAG name to validate
 * @return true if name is valid (4 ASCII chars, may contain '*' padding)
 */
[[nodiscard]] constexpr bool isValidTAGName(std::string_view name) noexcept {
    if (name.length() != constants::MAX_TAG_NAME_LENGTH) {
        return false;
    }
    for (char c : name) {
        if (!((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '*')) {
            return false;
        }
    }
    return true;
}

/**
 * Extract TAG packet from AF packet payload
 * @param af_packet AF packet containing TAG data
 * @return Extracted TAG packet
 */
[[nodiscard]] std::optional<TAGPacket> extractTAGPacket(const AFPacket& af_packet);

/**
 * Extract TAG packet from raw payload
 * @param payload Raw TAG packet data
 * @return Extracted TAG packet
 */
[[nodiscard]] std::optional<TAGPacket> extractTAGPacket(std::span<const uint8_t> payload);

/**
 * Parse AF packet from raw network data
 * @param data Raw packet data
 * @param length Data length
 * @return Parsed AF packet if valid
 */
[[nodiscard]] std::optional<AFPacket> parseAFPacket(const uint8_t* data, size_t length);

/**
 * Parse PFT packet from raw network data
 * @param data Raw packet data
 * @param length Data length
 * @return Parsed PFT packet if valid
 */
[[nodiscard]] std::optional<PFTPacket> parsePFTPacket(const uint8_t* data, size_t length);

/**
 * Auto-detect and parse EDI packet (AF or PFT)
 * @param data Raw packet data
 * @param length Data length
 * @return Parsed EDI packet with type detection
 */
[[nodiscard]] std::optional<EDIPacket> parseEDIPacket(const uint8_t* data, size_t length);

// ============================================================================
// Inline CRC Implementation
// ============================================================================

inline uint32_t AFPacket::calculateCRC() const noexcept {
    // CRC covers entire packet except CRC field itself
    std::vector<uint8_t> data;
    data.reserve(10 + payload.size());

    // Header
    data.push_back(static_cast<uint8_t>(sync_pattern >> 8));
    data.push_back(static_cast<uint8_t>(sync_pattern & 0xFF));
    data.push_back(static_cast<uint8_t>(packet_length >> 24));
    data.push_back(static_cast<uint8_t>((packet_length >> 16) & 0xFF));
    data.push_back(static_cast<uint8_t>((packet_length >> 8) & 0xFF));
    data.push_back(static_cast<uint8_t>(packet_length & 0xFF));
    data.push_back(static_cast<uint8_t>(sequence_number >> 8));
    data.push_back(static_cast<uint8_t>(sequence_number & 0xFF));
    data.push_back(protocol_version);
    data.push_back(protocol_type);

    // Payload
    data.insert(data.end(), payload.begin(), payload.end());

    return calculateCRC16(data.data(), data.size());
}

inline bool AFPacket::validateCRC() const noexcept {
    return calculateCRC() == crc;
}

inline uint16_t PFTPacket::calculateCRC() const noexcept {
    // Similar to AF packet - CRC covers entire packet except CRC field
    std::vector<uint8_t> data;
    data.reserve(16 + payload.size());

    // Header fields
    data.push_back(static_cast<uint8_t>(sync_pattern >> 8));
    data.push_back(static_cast<uint8_t>(sync_pattern & 0xFF));
    data.push_back(static_cast<uint8_t>(packet_length >> 8));
    data.push_back(static_cast<uint8_t>(packet_length & 0xFF));
    data.push_back(protocol_version);
    data.push_back(static_cast<uint8_t>((pseq >> 16) & 0xFF));
    data.push_back(static_cast<uint8_t>((pseq >> 8) & 0xFF));
    data.push_back(static_cast<uint8_t>(pseq & 0xFF));
    data.push_back(static_cast<uint8_t>((findex >> 16) & 0xFF));
    data.push_back(static_cast<uint8_t>((findex >> 8) & 0xFF));
    data.push_back(static_cast<uint8_t>(findex & 0xFF));
    data.push_back(static_cast<uint8_t>((fcount >> 16) & 0xFF));
    data.push_back(static_cast<uint8_t>((fcount >> 8) & 0xFF));
    data.push_back(static_cast<uint8_t>(fcount & 0xFF));
    data.push_back(fec_type);
    data.push_back(address_flag);

    // Optional addressing
    if (source_address.has_value()) {
        uint32_t addr = source_address.value();
        data.push_back(static_cast<uint8_t>((addr >> 24) & 0xFF));
        data.push_back(static_cast<uint8_t>((addr >> 16) & 0xFF));
        data.push_back(static_cast<uint8_t>((addr >> 8) & 0xFF));
        data.push_back(static_cast<uint8_t>(addr & 0xFF));
    }

    if (destination_address.has_value()) {
        uint32_t addr = destination_address.value();
        data.push_back(static_cast<uint8_t>((addr >> 24) & 0xFF));
        data.push_back(static_cast<uint8_t>((addr >> 16) & 0xFF));
        data.push_back(static_cast<uint8_t>((addr >> 8) & 0xFF));
        data.push_back(static_cast<uint8_t>(addr & 0xFF));
    }

    // Payload
    data.insert(data.end(), payload.begin(), payload.end());

    return calculateCRC16(data.data(), data.size());
}

inline bool PFTPacket::validateCRC() const noexcept {
    return calculateCRC() == crc;
}

} // namespace edi
