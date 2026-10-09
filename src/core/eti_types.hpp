/**
 * @file eti_types.h
 * @brief ETSI-Compliant ETI data type definitions and constants
 *
 * This file contains the fundamental data types, structures, and constants
 * used throughout the ETI Stream Analyser for DAB/DAB+ processing.
 * Fully compliant with ETSI EN 300 799 and ETSI EN 300 401 specifications.
 *
 * @author Standards Compliance Agent
 * @date 2025
 * @copyright Copyright (c) 2025 ETI Stream Analyser Team
 */

#ifndef ETI_TYPES_HPP
#define ETI_TYPES_HPP

#include <cstdint>
#include <array>
#include <vector>
#include <string>
#include <chrono>
#include <memory>
#include <map>
#include <cstring>
#include <algorithm>
#include <concepts>
#include <ranges>
#include <type_traits>
#include <QTime>
#include <QByteArray>
#include <cstdlib>
#include <cstring>
#include "crc16.hpp"

namespace eti {

// ETSI EN 300 799 Constants
constexpr size_t ETI_FRAME_SIZE = 6144;                    // bytes (ETSI EN 300 799)
constexpr size_t ETI_SYNC_SIZE = 4;                        // bytes
constexpr size_t ETI_LIDATA_SIZE = 8;                      // bytes
constexpr size_t ETI_FIC_SIZE = 32;                        // bytes
constexpr size_t ETI_MSC_SIZE = 6096;                      // bytes (6144 - 4 - 8 - 32 - 4)
constexpr size_t ETI_CRC_SIZE = 4;                         // bytes

constexpr uint32_t ETI_SAMPLE_RATE = 2048000;              // 2.048 MHz
constexpr uint32_t ETI_FRAMES_PER_SECOND = 250;            // 24ms per frame
constexpr std::chrono::microseconds ETI_FRAME_DURATION{24000}; // 24ms

// ETSI EN 300 401 Constants
constexpr size_t FIC_SIZE_BITS = 256;                      // bits
constexpr size_t FIC_SIZE_BYTES = 32;                      // bytes (one FIB)
constexpr size_t ETI_FIC_FIB_SIZE = 32;                    // bytes per FIB (Fast Information Block)
constexpr size_t ETI_FIC_FIG_AREA_SIZE = 30;               // FIG data area per FIB (FIB = 30B FIG + 2B CRC)
constexpr size_t ETI_FIC_MIN_SIZE = 96;                    // FIC length in Mode I/II/IV (3 FIBs)
constexpr size_t ETI_FIC_MAX_SIZE = 128;                   // FIC length in Mode III (4 FIBs)
constexpr size_t MAX_SUBCHANNEL_COUNT = 64;                // Maximum sub-channels
constexpr size_t MAX_CAPACITY_UNITS = 864;                 // Maximum CUs per frame

// Phase 1.1: Core ETI Processing Data Structures
/**
 * @brief Processed ETI Frame Information
 */
struct ProcessedFrame {
    quint64 frame_number;           // Sequential frame number
    uint32_t sync_pattern;          // SYNC field (4 bytes)
    uint32_t lidata;               // LIDATA field (4 bytes)
    QByteArray fic_data;           // FIC field (32 bytes)
    QByteArray msc_data;           // MSC field (variable size)
    uint32_t crc;                  // CRC field (4 bytes)
    QDateTime timestamp;           // Processing timestamp
    bool is_valid;                 // Frame validation status
};

/**
 * @brief FIG Information Structure
 */
struct FIGInfo {
    uint8_t type;                  // FIG type (0-31)
    uint8_t length;                // FIG data length
    QByteArray data;               // FIG data payload
    bool continuation;             // Continuation flag
    bool other_ensemble;           // Other ensemble flag
};
constexpr size_t CAPACITY_UNIT_SIZE = 8;                   // bytes per CU

// FIG Constants
constexpr size_t MAX_FIG_LENGTH = 29;                      // Maximum FIG data length
constexpr size_t FIG_HEADER_SIZE = 1;                      // bytes

// Sync pattern for ETI frames (ETSI EN 300 799)
// The Enhanced ETI processor accepts all three standard sync words; the
// receiver's packet validator must accept the same set so an ETI-LI-over-IP
// stream (e.g. the real Bangkok capture) is not rejected as "invalid".
constexpr std::array<uint8_t, 4> ETI_SYNC_PATTERN = {0x49, 0x93, 0x1E, 0x03};
constexpr std::array<uint8_t, 4> ETI_SYNC_LI_A   = {0xFF, 0xF8, 0xC5, 0x49};
constexpr std::array<uint8_t, 4> ETI_SYNC_LI_B   = {0xFF, 0x07, 0x3A, 0xB6};

/**
 * @brief ETI Format Types
 *
 * Automatic detection of ETI format based on sync patterns.
 * Reference: ETSI EN 300 799 Section 5.1
 */
enum class ETIFormat {
    ETI_LI,    // Linear (0x49 0x93 0x1E 0x03) - Most common format
    ETI_NI,    // Network Independent (0xFF 0xF8 or 0xFF 0x07) - Alternative format
    ETI_NA,    // Network Adapted (future support)
    UNKNOWN    // Invalid/unsupported sync pattern
};

/**
 * @brief Detect ETI format from frame data
 *
 * Analyzes the sync pattern to determine the ETI format variant.
 *
 * @param data Pointer to frame data (at least 4 bytes)
 * @param size Size of data buffer
 * @return ETIFormat Detected format type
 *
 * @note ETI-LI: 0x49 0x93 0x1E 0x03 (standard)
 * @note ETI-NI: 0xFF 0xF8 or 0xFF 0x07 (network independent variants)
 */
ETIFormat detectETIFormat(const uint8_t* data, size_t size);

/**
 * @brief Convert ETI format to string representation
 *
 * @param format ETI format enum value
 * @return const char* String name of format
 */
const char* formatToString(ETIFormat format);

// Forward declarations
struct EtiFrame;
struct FigBlock;
struct Ensemble;
struct DabService;
struct ServiceInfo;
struct ServiceComponent;
struct SubChannelInfo;

// ============================================================================
// C++20 CONCEPTS FOR TYPE SAFETY AND TEMPLATE CONSTRAINTS
// ============================================================================

/**
 * @brief Concept for ETI stream processing types
 *
 * Ensures types used for ETI stream processing have required interface
 * for frame data access, ensemble information, and validation.
 */
template<typename T>
concept eti_stream_type = requires(T t) {
    { t.get_frame_data() } -> std::convertible_to<std::vector<uint8_t>>;
    { t.get_ensemble_info() } -> std::convertible_to<Ensemble>;
    { t.is_valid() } -> std::convertible_to<bool>;
    { t.size() } -> std::convertible_to<size_t>;
};

/**
 * @brief Concept for DAB service types
 *
 * Ensures service types provide required service identification,
 * bitrate information, and activity status.
 */
template<typename T>
concept service_type = requires(T t) {
    { t.get_service_id() } -> std::convertible_to<uint16_t>;
    { t.get_bitrate() } -> std::convertible_to<uint16_t>;
    { t.is_active() } -> std::convertible_to<bool>;
    typename T::service_id_type;
};

/**
 * @brief Concept for ensemble container types
 *
 * Ensures ensemble containers support iteration over services
 * and provide ensemble-level information.
 */
template<typename T>
concept ensemble_container = requires(T t) {
    { t.get_services() } -> std::ranges::range;
    { t.get_ensemble_id() } -> std::convertible_to<uint16_t>;
    { t.get_label() } -> std::convertible_to<std::string>;
    { std::ranges::begin(t.get_services()) };
    { std::ranges::end(t.get_services()) };
};

/**
 * @brief Concept for ETI frame validation
 *
 * Ensures frame validation types provide comprehensive
 * ETSI compliance checking capabilities.
 */
template<typename T>
concept frame_validator = requires(T t, const EtiFrame& frame) {
    { t.validate_sync_pattern(frame) } -> std::convertible_to<bool>;
    { t.validate_frame_structure(frame) } -> std::convertible_to<bool>;
    { t.validate_crc(frame) } -> std::convertible_to<bool>;
    { t.get_validation_errors() } -> std::convertible_to<std::vector<std::string>>;
};

/**
 * @brief Concept for memory pool allocators
 *
 * Ensures memory pool types provide efficient allocation
 * and deallocation for ETI frame processing.
 */
template<typename T>
concept memory_allocator = requires(T t, size_t size, void* ptr) {
    { t.allocate(size) } -> std::convertible_to<void*>;
    { t.deallocate(ptr) } -> std::same_as<void>;
    { t.get_memory_usage() } -> std::convertible_to<size_t>;
    { t.optimize() } -> std::same_as<void>;
};

/**
 * @brief ETSI-compliant ETI Frame Sync Field
 * Reference: ETSI EN 300 799 Section 5.1
 */
struct eti_sync_field {
    std::array<uint8_t, 4> sync_bytes{};

    eti_sync_field() {
        set_sync_pattern();
    }

    bool is_valid() const {
        return std::memcmp(sync_bytes.data(), ETI_SYNC_PATTERN.data(), 4) == 0;
    }

    void set_sync_pattern() {
        std::memcpy(sync_bytes.data(), ETI_SYNC_PATTERN.data(), 4);
    }
};

/**
 * @brief ETSI-compliant ETI Frame LIDATA Field
 * Reference: ETSI EN 300 799 Section 5.2
 */
struct EtiLidataField {
    uint8_t fc;                 // Frame Count (0-249)
    uint8_t ficf : 1;          // FIC Flag (1=FIC present)
    uint8_t nst : 7;           // Number of Sub-channels in MSC (0-63)
    uint8_t fp : 3;            // Frame Phase (0-7)
    uint8_t mid : 2;           // Mode Identity (1-4)
    uint16_t fl : 11;          // Frame Length (11 bits fits in uint16_t)
    uint32_t tist;             // Time Stamp (32-bit) — LEGACY LI position (bytes 8-11).
                               // NOTE: ETI-NI/RAW uses the frame-tail TIST
                               // (get_tail_tist()); this LI-position read has not
                               // been validated against a real ETI-LI capture yet.

    bool validate_frame_count(uint8_t expected_fc) const {
        return fc == expected_fc || fc == ((expected_fc + 1) % 250);
    }

    bool validate_mode(uint8_t expected_mode) const {
        return mid == expected_mode;
    }

    std::chrono::microseconds get_timestamp_us() const {
        // TIST is in units of 16.384 MHz clock periods
        return std::chrono::microseconds(static_cast<uint64_t>(tist) * 1000000 / 16384000);
    }

    bool validate_nst() const {
        return nst <= MAX_SUBCHANNEL_COUNT;
    }
};

/**
 * @brief ETSI-compliant FIC Field with FIG support
 * Reference: ETSI EN 300 401 Section 6.4 / EN 300 799 Section 5.2
 *
 * Carries the complete Fast Information Channel (96 or 128 bytes = 3 or 4
 * FIBs). Each FIB is 32 bytes: 30 bytes of FIG data plus a 2-byte CRC-16
 * (complemented CCITT-FALSE over the 30 FIG bytes, big-endian).
 *
 * A 32-byte field (fib_count == 1) is treated as a legacy raw FIG area
 * (e.g. a FIC dump with the CRC stripped) for backwards compatibility.
 */

/**
 * @brief FIC decode mode (option-variant matrix row 4).
 *
 * Controls how the FIB-structured FIC (96/128 bytes) is turned into
 * FigBlock structures:
 * - Strict: validate every per-FIB CRC and skip corrupted FIBs (default
 *   historical behavior; bad FIBs counted in fib_crc_failures).
 * - Raw: treat the whole FIC area as one raw FIG stream (no per-FIB CRC),
 *   matching a plain 96-byte FIG dump or a full on-air FIC without FIB CRC
 *   (etisnoop `--ignore-errors`-equivalent escape, over the whole FIC).
 * - AutoFallback (default): behave like Strict; when EVERY FIB of the frame
 *   fails its CRC, re-run as Raw so a CRC-less FIC dump does not silently
 *   decode to zero FIGs. Only triggers when the strict walk finds nothing.
 */
enum class FicDecodeMode : uint8_t {
    Strict = 0,       // Validate per-FIB CRC, skip bad FIBs
    Raw = 1,          // Whole FIC as raw FIG stream, no per-FIB CRC
    AutoFallback = 2  // Strict; all-FIB-CRC-fail -> Raw (default)
};

/**
 * @brief Result of a mode-aware FIC decode.
 *
 * In addition to the decoded FIG blocks the result carries per-FIG / per-FIB
 * provenance and health so consumers can build chronological FIG traces and
 * aggregate FIC-health counters without re-walking the FIC (CLI detail wave,
 * P1: per-frame FIG instance trace + FIG inventory).
 */
struct FicDecodeResult {
    std::vector<FigBlock> fig_blocks;      // Decoded FIG blocks
    uint32_t fib_crc_failures{0};          // FIBs skipped in the strict walk
    bool raw_fallback_used{false};         // True when a raw walk produced the blocks

    // FIG provenance: parallel to fig_blocks. 0-based FIB index each FIG came
    // from (0 .. fib_count-1); 0xFF marks FIGs produced by a raw whole-FIC
    // walk (explicit Raw mode or the auto-fallback re-walk), which have no
    // per-FIB CRC.
    std::vector<uint8_t> fig_fib_indices;

    // FIB health in the strict walk: bit i of fib_crc_ok_mask is set when
    // FIB i's CRC validated OK (a legacy 32-byte raw field has no CRC and
    // sets bit 0). fib_count is the number of FIBs (or legacy fields) walked.
    uint8_t fib_crc_ok_mask{0};
    uint8_t fib_count{0};

    // FIG headers skipped because of length/padding anomalies (length 0,
    // length overflowing the FIB area, or an out-of-spec length > 29).
    uint32_t erroneous_figs{0};
};

struct EtiFicField {
    std::array<uint8_t, ETI_FIC_MAX_SIZE> fic_data;
    uint16_t fic_size;            // Number of valid bytes (0, 32, 96 or 128)
    uint8_t fib_count;            // Number of complete FIBs carried (0, 1, 3 or 4)

    EtiFicField() : fic_data{}, fic_size(FIC_SIZE_BYTES), fib_count(1) {
    }

    explicit EtiFicField(const std::array<uint8_t, FIC_SIZE_BYTES>& data)
        : fic_data{}, fic_size(FIC_SIZE_BYTES), fib_count(1) {
        std::memcpy(fic_data.data(), data.data(), FIC_SIZE_BYTES);
    }

    EtiFicField(const uint8_t* data, size_t size)
        : fic_data{}, fic_size(0), fib_count(0) {
        if (data == nullptr || size == 0) {
            return;
        }
        const size_t n = std::min(size, ETI_FIC_MAX_SIZE);
        std::memcpy(fic_data.data(), data, n);
        fic_size = static_cast<uint16_t>(n);
        fib_count = static_cast<uint8_t>(n / ETI_FIC_FIB_SIZE);
    }

    /** @brief Number of valid bytes carried by this field. */
    size_t size() const { return fic_size; }

    /** @brief Number of FIBs carried (1 for raw 32-byte legacy data). */
    size_t fibi_count() const {
        if (fib_count > 0) {
            return fib_count;
        }
        return fic_size / ETI_FIC_FIB_SIZE;
    }

    /** @brief True when the field carries complete FIB structure (96/128 B). */
    bool is_fib_structured() const {
        return fic_size > ETI_FIC_FIB_SIZE && fibi_count() >= 3;
    }

    /**
     * @brief Decode FIG blocks honouring the FIC decode mode.
     *
     * The legacy 32-byte field is always walked as a single raw FIG area.
     * For FIB-structured FICs, @p mode selects strict FIB-CRC validation,
     * a forced raw walk, or the auto-fallback default (raw re-walk only
     * when every FIB CRC fails). Honors the DABX_FIC_MODE=strict|raw
     * environment override for the AutoFallback path (testing hook; the
     * settings feature supersedes it via @p mode).
     *
     * @p ignore_fib_crc (settings row 5) keeps the strict FIB walk but skips
     * the CRC gate: FIBs with a failing CRC are still decoded (failures are
     * counted for diagnostics). The auto-fallback re-walk still applies when
     * every FIB CRC fails, producing the same blocks for well-formed FICs.
     *
     * @param mode Requested decode mode (default: AutoFallback).
     * @param ignore_fib_crc When true, decode FIBs whose CRC fails instead of
     *                       dropping them (default: false).
     * @return Decoded blocks plus FIB-CRC / fallback diagnostics.
     */
    FicDecodeResult decodeFigBlocks(
        FicDecodeMode mode = FicDecodeMode::AutoFallback,
        bool ignore_fib_crc = false) const;

    std::vector<FigBlock> decode_fig_blocks() const;
    bool validate_fic_crc() const;
    bool contains_fig_type(uint8_t fig_type) const;
};

/**
 * @brief FIG Block structure for FIC analysis
 * Reference: ETSI EN 300 401 Section 5.2
 */
struct FigBlock {
    uint8_t fig_type;             // FIG type (0-7; 7 = padding end marker)
    uint8_t length;               // FIG data length in bytes (0-29)
    bool continuation_flag = false;     // C: Continuation flag
    bool other_ensemble_flag = false;   // OE: Other Ensemble flag
    std::vector<uint8_t> data;    // FIG data field

    bool is_valid() const {
        return fig_type <= 7 && length <= MAX_FIG_LENGTH &&
               data.size() == length;
    }

    uint8_t get_extension() const {
        return data.empty() ? 0 : (data[0] & 0x1F);
    }
};

// ============================================================================
// FIC decode helpers (header-inline so fig_parser.cpp consumers link without
// a separate eti_types.cpp translation unit — single shared implementation,
// option-variant matrix row 4).
// ============================================================================

/**
 * @brief Expected (complemented) FIB CRC over a 30-byte FIG area.
 *
 * CRC-16/CCITT-FALSE shared implementation lives in crc16.hpp (F6 dedup);
 * the transmitted FIB CRC is the one's complement, big-endian.
 */
inline uint16_t fib_crc_expected(const uint8_t* fig_area)
{
    return static_cast<uint16_t>(~crc16ccitt_false(fig_area, ETI_FIC_FIG_AREA_SIZE));
}

/**
 * @brief Parse FIG blocks from one FIG area (30-byte FIB area or whole FIC).
 *
 * FIG header: type (3 bits, b7-b5; 7 = padding), length (5 bits, b4-b0).
 * FIGs never span FIB boundaries (ETSI EN 300 401 clause 5.2); the raw walk
 * over a whole FIC uses the same rules and ends on type 7 or the area bound.
 *
 * @param area         FIG area bytes
 * @param area_size    Valid bytes in @p area
 * @param erroneous    Optional out-counter for FIG headers skipped due to
 *                     length/padding anomalies (length 0, truncation, or an
 *                     out-of-spec length > 29). Never null-dereferenced.
 */
inline std::vector<FigBlock> parse_fig_area(const uint8_t* area, size_t area_size,
                                            uint32_t* erroneous = nullptr)
{
    std::vector<FigBlock> fig_blocks;
    size_t offset = 0;

    while (offset + 1 <= area_size) {
        const uint8_t header = area[offset];
        const uint8_t fig_type = (header >> 5) & 0x07;

        if (fig_type == 7) {
            // Padding end marker
            break;
        }

        const uint8_t length = header & 0x1F;

        if (length == 0 || offset + 1 + length > area_size) {
            // Invalid or truncated FIG — stop processing this area. Count the
            // anomaly for FIC-health diagnostics (the walk itself is unchanged).
            if (erroneous != nullptr) {
                ++(*erroneous);
            }
            break;
        }

        FigBlock fig_block;
        fig_block.fig_type = fig_type;
        fig_block.length = length;

        // Out-of-spec lengths (30/31, > MAX_FIG_LENGTH): the historical walk
        // skips such blocks via is_valid() but keeps advancing; only count the
        // anomaly for diagnostics, never change the produced blocks.
        if (length > MAX_FIG_LENGTH) {
            if (erroneous != nullptr) {
                ++(*erroneous);
            }
            offset += 1 + length;
            continue;
        }

        fig_block.data.assign(area + offset + 1, area + offset + 1 + length);

        // FIG 0 header extension bits: C/N (b7), O/E (b6) of the ext byte
        if (fig_type == 0 && !fig_block.data.empty()) {
            fig_block.continuation_flag = (fig_block.data[0] & 0x80) != 0;
            fig_block.other_ensemble_flag = (fig_block.data[0] & 0x40) != 0;
        }

        if (fig_block.is_valid()) {
            fig_blocks.push_back(std::move(fig_block));
        }

        offset += 1 + length;
    }

    return fig_blocks;
}

/**
 * @brief Mode-aware FIC -> FIG decode (see FicDecodeMode for semantics).
 */
inline FicDecodeResult EtiFicField::decodeFigBlocks(FicDecodeMode mode, bool ignore_fib_crc) const {
    FicDecodeResult result;

    // Resolve the mode: the AutoFallback default honors the DABX_FIC_MODE
    // environment override (strict|raw) as a pre-settings testing hook.
    // Explicit Strict/Raw requests from callers are never overridden.
    FicDecodeMode effective = mode;
    if (mode == FicDecodeMode::AutoFallback) {
        if (const char* env = std::getenv("DABX_FIC_MODE")) {
            if (std::strcmp(env, "strict") == 0) {
                effective = FicDecodeMode::Strict;
            } else if (std::strcmp(env, "raw") == 0) {
                effective = FicDecodeMode::Raw;
            }
        }
    }

    const bool fib_structured = is_fib_structured();

    // Explicit raw mode: walk the whole FIC area as one FIG stream.
    if (effective == FicDecodeMode::Raw) {
        result.raw_fallback_used = true;
        result.fig_blocks = parse_fig_area(fic_data.data(), fic_size,
                                           &result.erroneous_figs);
        // Raw walk: no per-FIB structure; mark FIG provenance as unknown.
        result.fig_fib_indices.assign(result.fig_blocks.size(), 0xFF);
        result.fib_count = 0;
        return result;
    }

    // Strict FIB walk (byte-identical to the historical behavior for every
    // input): a FIB-structured FIC (96/128 bytes) carries 3/4 FIBs and every
    // FIB CRC is validated, corrupted FIBs skipped. A legacy 32-byte field is
    // walked as a single 30-byte raw FIG area.
    const size_t fib_count_effective = fib_structured ? fibi_count() : 1;
    result.fib_count = static_cast<uint8_t>(fib_count_effective);
    uint32_t per_fib_erroneous = 0;
    for (size_t fib = 0; fib < fib_count_effective; ++fib) {
        const size_t fib_base = fib * ETI_FIC_FIB_SIZE;
        if (fib_base + ETI_FIC_FIB_SIZE > fic_size) {
            break;
        }
        const uint8_t* fib_data = fic_data.data() + fib_base;

        bool fib_crc_ok = true;
        if (fib_structured) {
            const uint16_t stored_crc = static_cast<uint16_t>((fib_data[30] << 8) | fib_data[31]);
            if (fib_crc_expected(fib_data) != stored_crc) {
                ++result.fib_crc_failures;
                fib_crc_ok = false;
                if (!ignore_fib_crc) {
                    continue;
                }
            }
        }
        if (fib_crc_ok) {
            result.fib_crc_ok_mask |= static_cast<uint8_t>(1u << fib);
        }

        per_fib_erroneous = 0;
        auto fib_figs = parse_fig_area(fib_data, ETI_FIC_FIG_AREA_SIZE,
                                       &per_fib_erroneous);
        result.erroneous_figs += per_fib_erroneous;
        result.fig_fib_indices.reserve(result.fig_fib_indices.size() + fib_figs.size());
        for (size_t i = 0; i < fib_figs.size(); ++i) {
            result.fig_fib_indices.push_back(static_cast<uint8_t>(fib));
        }
        result.fig_blocks.insert(result.fig_blocks.end(),
                                 std::make_move_iterator(fib_figs.begin()),
                                 std::make_move_iterator(fib_figs.end()));
    }

    // Auto fallback: when every FIB of the frame failed its CRC the FIC is
    // most likely a CRC-less dump (96-byte FIG stream or full on-air FIC).
    // Re-run the same FIG-walk over the whole FIC so FIGs are not lost.
    // With ignore_fib_crc the per-FIB walk already decoded every FIB; the
    // re-walk produces the same blocks for well-formed FICs and the correct
    // ones for genuine CRC-less dumps, so the fallback still applies.
    if (effective == FicDecodeMode::AutoFallback &&
        fib_structured &&
        result.fib_crc_failures == fib_count_effective) {
        result.raw_fallback_used = true;
        const auto fallback_blocks = parse_fig_area(fic_data.data(), fic_size,
                                                    &result.erroneous_figs);
        result.fig_blocks = fallback_blocks;
        result.fig_fib_indices.assign(result.fig_blocks.size(), 0xFF);
    }

    return result;
}

/**
 * @brief MSC Sub-channel Information
 * Reference: ETSI EN 300 401 Section 6.2
 */
struct SubChannelInfo {
    uint8_t sub_channel_id;     // SubChId (0-63)
    uint16_t start_address;     // Start address in CUs
    uint16_t size;              // Size in CUs
    uint8_t protection_level;   // Protection level (1-based; EEP: 1-4, UEP: derived)
    bool uep_flag;             // Unequal Error Protection flag
    // EEP protection option (EN 300 401 8.1.2.1 long form / STC TPL bits 4-2):
    // 0 = EEP-x-A, 1 = EEP-x-B. Meaningless for UEP short form.
    uint8_t protection_option{0};

    // Short-form (UEP) protection table selector (EN 300 401 8.1.2.1 / STC
    // TPL): table_switch (1 bit) + table_index (6 bits in FIG 0/1, 3 bits in
    // the STC TPL). The UEP sub-channel size is derived from the protection
    // table (protection_tables.hpp), not signalled in the stream.
    uint8_t table_switch{0};
    uint8_t table_index{0};

    uint16_t get_end_address() const {
        return start_address + size - 1;
    }

    bool validate_boundaries() const {
        return (start_address + size) <= MAX_CAPACITY_UNITS;
    }

    bool validate_sub_channel_id() const {
        return sub_channel_id < MAX_SUBCHANNEL_COUNT;
    }

    bool validate_protection_level() const {
        return protection_level >= 1 && protection_level <= 5;
    }
};

/**
 * @brief ETSI-compliant ETI Frame MSC Field
 * Reference: ETSI EN 300 799 Section 5.3
 */
struct EtiMscField {
    std::array<uint8_t, ETI_MSC_SIZE> msc_data;

    EtiMscField() : msc_data{} {
        msc_data.fill(0);
    }

    explicit EtiMscField(const std::array<uint8_t, ETI_MSC_SIZE>& data) : msc_data(data) {
    }

    std::vector<uint8_t> extract_subchannel_data(const SubChannelInfo& subchannel) const;
    bool validate_subchannel_organization(const std::vector<SubChannelInfo>& subchannels) const;

    size_t get_capacity_units_count() const {
        return ETI_MSC_SIZE / CAPACITY_UNIT_SIZE;  // 762 CUs
    }
};

/**
 * @brief ETSI-compliant ETI Frame CRC Field
 * Reference: ETSI EN 300 799 Section 5.4
 */
struct EtiCrcField {
    std::array<uint8_t, 4> crc_bytes{};

    EtiCrcField() {
        crc_bytes.fill(0);
    }

    bool validate_frame(const EtiFrame& frame) const;
    void calculate_crc(const EtiFrame& frame);
    uint32_t get_crc_value() const {
        return (static_cast<uint32_t>(crc_bytes[0]) << 24) |
               (static_cast<uint32_t>(crc_bytes[1]) << 16) |
               (static_cast<uint32_t>(crc_bytes[2]) << 8) |
               static_cast<uint32_t>(crc_bytes[3]);
    }

private:
    uint32_t calculate_frame_crc(const EtiFrame& frame) const;
};

/**
 * @brief Enhanced Service Component with ETSI compliance
 * Reference: ETSI EN 300 401 Section 8.1
 */
struct ServiceComponent {
    uint8_t sub_channel_id{0};     // Associated sub-channel
    uint16_t service_id{0};        // Parent service ID
    uint8_t component_type{0};     // Component type (audio, data, etc.)
    uint8_t tmid : 2 {0};          // Transport Mechanism ID
    uint8_t asc_ty : 6 {0};        // Audio Service Component Type
    bool primary{false};               // Primary/Secondary component
    bool ca_flag{false};              // Conditional Access flag
    uint8_t protection_level{0};   // Error protection level (0-31)
    std::string label;          // Component label

    // Default constructor with proper initialization
    ServiceComponent() = default;

    bool validate_tmid() const {
        return tmid <= 3;
    }

    bool validate_asc_ty() const {
        return asc_ty <= 63;
    }

    bool validate_sub_channel_id() const {
        return sub_channel_id < MAX_SUBCHANNEL_COUNT;
    }
};

/**
 * @brief Enhanced DAB Service with ETSI compliance
 * Reference: ETSI EN 300 401 Section 8.1
 */
struct DabService {
    uint16_t service_id{0};            // SId: Service identifier (16-bit programme SId,
                                       // or SRef = low 16 bits of a 32-bit data SId)
    uint32_t sid32{0};                 // Full 32-bit SId for data services (PD=1):
                                       // [ECC 8][CId(4)|SRef-high(4)][SRef 16]; 0 for programme services
    uint8_t country_id{0};             // Country ID
    uint8_t extended_country_code{0};  // ECC: Extended Country Code
    std::string label;                 // Service label (UTF-8, full 16-char trimmed)
    std::string short_label;           // Short label derived from the 16-bit
                                       // character flag mask (EN 300 401 8.1.14),
                                       // populated by the FIG 1/1 parser; empty
                                       // when no mask was carried (settings row 8)
    std::vector<ServiceComponent> components;
    bool is_programme{false};          // Programme/Data service flag
    uint16_t character_flag{0};        // Character definition field (mask)

    bool validate_service_id() const {
        // Data services (PD=1) carry a full 32-bit SId in `sid32`
        // ([ECC][CId|SRef-hi][SRef]); their low 16 bits (SRef) may legitimately
        // be 0x0000 (e.g. 0xF3200000), so validate the full value there instead
        // of the truncated 16-bit SRef. Programme services keep the historical
        // 16-bit rule (0x0000/0xFFFF invalid).
        // Data-vs-programme is inferred from `sid32` (data services set it;
        // programme services leave it 0) — keep in sync with FIG 0/2 P/D.
        if (sid32 != 0) {
            return sid32 != 0xFFFFFFFFu;
        }
        return service_id != 0x0000 && service_id != 0xFFFF;
    }

    bool validate_country_code() const {
        return country_id != 0x00 && country_id != 0xFF;
    }

    bool has_primary_component() const {
        return std::any_of(components.begin(), components.end(),
                          [](const ServiceComponent& comp) { return comp.primary; });
    }

    std::vector<uint8_t> get_subchannel_ids() const {
        std::vector<uint8_t> subchannel_ids;
        subchannel_ids.reserve(components.size()); // Pre-allocate for performance
        for (const auto& component : components) {
            subchannel_ids.push_back(component.sub_channel_id);
        }
        return subchannel_ids;
    }
};

/**
 * @brief Enhanced Ensemble with ETSI compliance
 * Reference: ETSI EN 300 401 Section 6.4
 */
struct Ensemble {
    uint16_t ensemble_id{0};           // EId: Ensemble identifier
    std::string label;                 // Ensemble label (UTF-8, full 16-char trimmed)
    std::string short_label;           // Short label from the 16-bit mask
                                       // (EN 300 401 8.1.14), empty when absent
    std::vector<DabService> services;  // Services in ensemble
    std::vector<SubChannelInfo> sub_channels; // Sub-channel organization
    uint8_t country_id{0};             // Country ID
    uint8_t extended_country_code{0};  // ECC: Extended Country Code
    uint16_t character_flag{0};        // Character definition field (mask)
    uint16_t cif_count{0};             // Current CIF count
    uint8_t occurrence_change{0};      // Occurrence change field
    bool alarm_flag{false};            // Alarm announcement flag

    bool validate_ensemble_id() const {
        return ensemble_id != 0x0000 && ensemble_id != 0xFFFF;
    }

    bool validate_country_code() const {
        return country_id != 0x00 && country_id != 0xFF;
    }

    bool validate_subchannel_organization() const {
        // Check for overlapping sub-channels
        for (size_t i = 0; i < sub_channels.size(); ++i) {
            for (size_t j = i + 1; j < sub_channels.size(); ++j) {
                const auto& a = sub_channels[i];
                const auto& b = sub_channels[j];
                if (!(a.start_address + a.size <= b.start_address ||
                      b.start_address + b.size <= a.start_address)) {
                    return false;  // Overlap detected
                }
            }
        }
        return true;
    }

    uint16_t calculate_total_capacity() const {
        uint16_t total = 0;
        for (const auto& sub_ch : sub_channels) {
            total += sub_ch.size;
        }
        return total;
    }

    std::vector<uint16_t> get_service_ids() const {
        std::vector<uint16_t> service_ids;
        service_ids.reserve(services.size()); // Pre-allocate for performance
        for (const auto& service : services) {
            service_ids.push_back(service.service_id);
        }
        return service_ids;
    }
};

/**
 * @brief Complete ETSI-compliant ETI Frame
 * Reference: ETSI EN 300 799
 */
struct EtiFrame {
    std::array<uint8_t, ETI_FRAME_SIZE> frame_data;
    bool frame_valid;
    std::chrono::system_clock::time_point receive_timestamp;
    // Number of valid FIC bytes stored at the normalized offset 12
    // (96 or 128 for a full FIB-structured FIC; 0 = not set / legacy 32 B view)
    uint16_t fic_field_size{0};
    // Tail Time Stamp (EN 300 799 clause 5.3.6): for RAW/NI frames the
    // 32-bit TIST is carried in the last 4 bytes of the frame (6140..6143),
    // NOT in bytes 8-11 (that is a legacy ETI-LI misconception). Populated
    // by the ETI-NI parse path (see get_tail_tist()).
    uint32_t tist_tail{0};
    bool tist_tail_valid{false};

    EtiFrame() : frame_valid(false) {
        frame_data.fill(0);
        receive_timestamp = std::chrono::system_clock::now();
    }

    explicit EtiFrame(const std::array<uint8_t, ETI_FRAME_SIZE>& data)
        : frame_data(data), frame_valid(false) {
        receive_timestamp = std::chrono::system_clock::now();
        initialize_frame_validation();
    }

    // Field accessors
    eti_sync_field get_sync_field() const {
        eti_sync_field sync;
        std::memcpy(sync.sync_bytes.data(), frame_data.data(), 4);
        return sync;
    }

    EtiLidataField get_lidata_field() const {
        EtiLidataField lidata{};
        const uint8_t* data = frame_data.data() + 4;

        // Parse LIDATA field according to ETSI EN 300 799 Section 5.2
        lidata.fc = data[0];                    // Frame Count
        lidata.ficf = (data[1] & 0x80) ? 1 : 0; // FIC Flag
        lidata.nst = data[1] & 0x7F;            // Number of Sub-channels
        lidata.fp = (data[2] >> 5) & 0x07;      // Frame Phase
        lidata.mid = (data[2] >> 3) & 0x03;     // Mode Identity
        lidata.fl = ((data[2] & 0x07) << 8) | data[3]; // Frame Length

        // Time Stamp (TIST) - 32-bit value
        // DEPRECATED for ETI-NI/RAW frames: bytes 8-11 only carry the TIST
        // under the legacy ETI-LI layout assumption. Per EN 300 799 clause
        // 5.3.6 the timestamp sits at the frame tail; the NI path must use
        // get_tail_tist() / get_tail_tist_ms() instead.
        lidata.tist = (static_cast<uint32_t>(data[4]) << 24) |
                      (static_cast<uint32_t>(data[5]) << 16) |
                      (static_cast<uint32_t>(data[6]) << 8) |
                      static_cast<uint32_t>(data[7]);

        return lidata;
    }

    /**
     * @brief Read the frame-tail TIST (EN 300 799 clause 5.3.6).
     *
     * RAW/NI frames place the 32-bit time stamp in the last 4 bytes of the
     * 6144-byte frame (6140..6143), after ...MSC + EOF CRC(2) + RFU(2).
     *
     * @return Raw 32-bit TIST value read big-endian from the frame tail.
     */
    uint32_t get_tail_tist() const {
        const uint8_t* p = frame_data.data() + (ETI_FRAME_SIZE - 4);
        return (static_cast<uint32_t>(p[0]) << 24) |
               (static_cast<uint32_t>(p[1]) << 16) |
               (static_cast<uint32_t>(p[2]) << 8) |
               static_cast<uint32_t>(p[3]);
    }

    /**
     * @brief Tail TIST converted to milliseconds (etisnoop parity).
     *
     * etisnoop (etianalyse.cpp "TIST") prints the 24-bit payload as
     * `(TIST & 0xFFFFFF) / 16384.0` ms.
     *
     * @return Tail timestamp in milliseconds (double).
     */
    double get_tail_tist_ms() const {
        return static_cast<double>(get_tail_tist() & 0x00FFFFFF) / 16384.0;
    }

    EtiFicField get_fic_field() const {
        EtiFicField fic;
        // The modern parsers normalize the full FIC (96/128 bytes) to offset 12
        // and record the valid length in fic_field_size. Fall back to the legacy
        // 32-byte single-FIB view when the length was never recorded.
        const size_t n = (fic_field_size > 0)
            ? std::min<size_t>(fic_field_size, ETI_FIC_MAX_SIZE)
            : FIC_SIZE_BYTES;
        std::memcpy(fic.fic_data.data(), frame_data.data() + 12, n);
        fic.fic_size = static_cast<uint16_t>(n);
        fic.fib_count = static_cast<uint8_t>(n / ETI_FIC_FIB_SIZE);
        return fic;
    }

    EtiMscField get_msc_field() const {
        EtiMscField msc;
        std::memcpy(msc.msc_data.data(), frame_data.data() + 44, ETI_MSC_SIZE);
        return msc;
    }

    EtiCrcField get_crc_field() const {
        EtiCrcField crc;
        std::memcpy(crc.crc_bytes.data(), frame_data.data() + ETI_FRAME_SIZE - 4, 4);
        return crc;
    }

    // Validation methods
    bool validate_sync_pattern() const {
        auto sync = get_sync_field();
        return sync.is_valid();
    }

    bool validate_frame_structure() const {
        bool valid = validate_sync_pattern();
        if (valid) {
            auto crc = get_crc_field();
            valid = crc.validate_frame(*this);
        }
        return valid;
    }

    bool is_valid() const {
        return frame_valid;
    }

    // Frame analysis
    std::vector<FigBlock> decode_fic_blocks() const {
        auto fic = get_fic_field();
        return fic.decode_fig_blocks();
    }

    std::vector<SubChannelInfo> get_subchannel_info() const;
    Ensemble extract_ensemble_info() const;

    // Raw data access
    const uint8_t* data() const {
        return frame_data.data();
    }

    uint8_t* data() {
        return frame_data.data();
    }

    size_t size() const {
        return frame_data.size();
    }

    // Timing information
    void set_receive_timestamp(std::chrono::system_clock::time_point timestamp) {
        receive_timestamp = timestamp;
    }

    std::chrono::system_clock::time_point get_receive_timestamp() const {
        return receive_timestamp;
    }

    std::chrono::microseconds get_frame_timestamp() const {
        auto lidata = get_lidata_field();
        return lidata.get_timestamp_us();
    }

private:
    void initialize_frame_validation() {
        // Basic validation - check if frame has minimum required size
        frame_valid = (frame_data.size() >= ETI_SYNC_SIZE + ETI_LIDATA_SIZE);
    }

    // FIG parsing helper methods
    void parse_fig_0_0(const FigBlock& fig, Ensemble& ensemble) const;
    void parse_fig_0_1(const FigBlock& fig, std::vector<SubChannelInfo>& subchannels) const;
    void parse_fig_0_2(const FigBlock& fig, Ensemble& ensemble) const;
    void parse_fig_0_3(const FigBlock& fig, Ensemble& ensemble) const;
    void parse_fig_1_0(const FigBlock& fig, Ensemble& ensemble) const;
    void parse_fig_1_1(const FigBlock& fig, Ensemble& ensemble) const;
};

// Type aliases for compatibility and convenience
using EtiFrameArray = std::array<uint8_t, ETI_FRAME_SIZE>;
using FicBlockArray = std::array<uint8_t, FIC_SIZE_BYTES>;

/**
 * @brief Simple ETI frame data structure for decoder interfaces
 */
struct EtiFrameData {
    quint32 frameNumber{0};
    QTime timestamp;
    QByteArray data;
    bool isValid{false};

    EtiFrameData() = default;
};

/**
 * @brief ETI Frame validation result
 */
struct ValidationResult {
    bool valid;
    std::vector<std::string> errors;
    std::vector<std::string> warnings;

    ValidationResult(bool v = true) : valid(v) {}

    void add_error(const std::string& error) {
        errors.push_back(error);
        valid = false;
    }

    void add_warning(const std::string& warning) {
        warnings.push_back(warning);
    }

    bool has_errors() const {
        return !errors.empty();
    }

    bool has_warnings() const {
        return !warnings.empty();
    }
};

/**
 * @brief Service Type enumeration for DAB services
 * Reference: ETSI EN 300 401 Section 8.1
 */
enum class ServiceType : uint8_t {
    DAB_AUDIO = 0x00,       // DAB audio service
    DAB_PLUS_AUDIO = 0x01,  // DAB+ audio service (HE-AAC)
    DATA_SERVICE = 0x02,    // Data service
    PACKET_MODE = 0x03,     // Packet mode data
    MSC_DATA = 0x04,        // MSC stream data
    FIDC = 0x05,           // Fast Information Data Channel
    UNKNOWN = 0xFF          // Unknown/invalid service type
};

/**
 * @brief Service Information structure for testing and analysis
 * This structure provides a simplified interface for service discovery tests
 * while maintaining compatibility with the full DabService structure.
 */
struct ServiceInfo {
    uint16_t service_id;        // Service identifier (SId)
    std::string label;          // Service name/label (changed from 'name' to match test expectations)
    ServiceType service_type;   // Type of service
    uint16_t bitrate;          // Service bitrate in kbps
    uint8_t protection_level;   // Protection level (1-5)

    // Constructor with default values
    ServiceInfo()
        : service_id(0)
        , label()
        , service_type(ServiceType::UNKNOWN)
        , bitrate(0)
        , protection_level(1)
    {
    }

    // Constructor for easy initialization in tests
    ServiceInfo(uint16_t sid, const std::string& service_label,
                ServiceType type, uint16_t br, uint8_t protection)
        : service_id(sid)
        , label(service_label)
        , service_type(type)
        , bitrate(br)
        , protection_level(protection)
    {
    }

    // Validation methods
    bool is_valid() const {
        return service_id != 0 &&
               service_type != ServiceType::UNKNOWN &&
               protection_level >= 1 && protection_level <= 5;
    }

    // Type checking methods
    bool is_audio_service() const {
        return service_type == ServiceType::DAB_AUDIO ||
               service_type == ServiceType::DAB_PLUS_AUDIO;
    }

    bool is_data_service() const {
        return service_type == ServiceType::DATA_SERVICE ||
               service_type == ServiceType::PACKET_MODE ||
               service_type == ServiceType::MSC_DATA ||
               service_type == ServiceType::FIDC;
    }

    bool is_dab_plus() const {
        return service_type == ServiceType::DAB_PLUS_AUDIO;
    }

    // Convert to full DabService structure
    DabService to_dab_service() const {
        DabService service;
        service.service_id = service_id;
        service.label = label;
        service.is_programme = is_audio_service();
        return service;
    }

    // ============================================================================
    // C++20 STRUCTURED BINDINGS SUPPORT
    // ============================================================================

    /**
     * @brief Get service identification tuple for structured bindings
     * @return [service_id, label] tuple
     *
     * Usage: auto [id, name] = service.get_identification();
     */
    [[nodiscard]] auto get_identification() const noexcept -> std::pair<uint16_t, std::string> {
        return {service_id, label};
    }

    /**
     * @brief Get service characteristics tuple for structured bindings
     * @return [service_type, bitrate, protection_level] tuple
     *
     * Usage: auto [type, rate, protection] = service.get_characteristics();
     */
    [[nodiscard]] auto get_characteristics() const noexcept
        -> std::tuple<ServiceType, uint16_t, uint8_t> {
        return {service_type, bitrate, protection_level};
    }

    /**
     * @brief Validate service data with detailed error reporting
     * @return [is_valid, error_message] pair
     *
     * Usage: auto [valid, error] = service.validate_with_details();
     */
    [[nodiscard]] auto validate_with_details() const noexcept
        -> std::pair<bool, std::string> {
        if (service_id == 0) {
            return {false, "Invalid service ID: cannot be zero"};
        }
        if (service_type == ServiceType::UNKNOWN) {
            return {false, "Unknown service type"};
        }
        if (protection_level < 1 || protection_level > 5) {
            return {false, "Invalid protection level: must be 1-5"};
        }
        if (label.empty()) {
            return {false, "Service label cannot be empty"};
        }
        return {true, ""};
    }
};

/**
 * @brief Protection level enumeration
 * Reference: ETSI EN 300 401 Section 11.1
 */
enum class ProtectionLevel : uint8_t {
    LEVEL_1 = 1,    // Code rate 1/4, highest protection
    LEVEL_2 = 2,    // Code rate 3/8
    LEVEL_3 = 3,    // Code rate 1/2
    LEVEL_4 = 4,    // Code rate 3/4
    LEVEL_5 = 5     // Code rate 1, lowest protection
};

/**
 * @brief FIG type enumeration for type safety
 */
enum class FigType : uint8_t {
    FIG_0_0_ENSEMBLE_INFO = 0x00,
    FIG_0_1_SUBCHANNEL_ORG = 0x01,
    FIG_0_2_SERVICE_ORG = 0x02,
    FIG_0_3_SERVICE_COMPONENT = 0x03,
    FIG_1_0_ENSEMBLE_LABEL = 0x10,
    FIG_1_1_SERVICE_LABEL = 0x11,
    // Additional FIG types as needed
};

/**
 * @brief ETSI Compliance Test Result
 * Used for testing and validation of ETSI compliance features
 */
struct ETSIComplianceResult {
    bool is_compliant{false};                    // Overall compliance status
    double compliance_score{0.0};              // Compliance score (0.0-100.0)
    std::vector<std::string> errors;      // Compliance errors found
    std::vector<std::string> warnings;    // Compliance warnings
    std::string etsi_standard{"EN 300 401"};           // ETSI standard reference (e.g., "EN 300 401")
    std::string test_date;               // ISO date string of test

    // Constructor
    ETSIComplianceResult() = default;

    // Add error with automatic compliance status update
    void add_error(const std::string& error) {
        errors.push_back(error);
        is_compliant = false;
        recalculate_score();
    }

    // Add warning
    void add_warning(const std::string& warning) {
        warnings.push_back(warning);
        recalculate_score();
    }

    // Set compliance with score
    void set_compliance(bool compliant, double score = 100.0) {
        is_compliant = compliant;
        compliance_score = score;
    }

    // Calculate compliance percentage
    double get_compliance_percentage() const {
        return compliance_score;
    }

    // Check if fully compliant (100%)
    bool is_fully_compliant() const {
        return is_compliant && compliance_score >= 100.0 && errors.empty();
    }

    // ============================================================================
    // C++20 STRUCTURED BINDINGS SUPPORT FOR COMPLIANCE RESULTS
    // ============================================================================

    /**
     * @brief Get compliance status for structured bindings
     * @return [is_compliant, compliance_score] pair
     *
     * Usage: auto [compliant, score] = result.get_compliance_status();
     */
    [[nodiscard]] auto get_compliance_status() const noexcept
        -> std::pair<bool, double> {
        return {is_compliant, compliance_score};
    }

    /**
     * @brief Get error and warning counts for structured bindings
     * @return [error_count, warning_count] pair
     *
     * Usage: auto [errors, warnings] = result.get_issue_counts();
     */
    [[nodiscard]] auto get_issue_counts() const noexcept
        -> std::pair<size_t, size_t> {
        return {errors.size(), warnings.size()};
    }

    /**
     * @brief Get compliance summary for structured bindings
     * @return [is_compliant, score, error_count, warning_count] tuple
     *
     * Usage: auto [compliant, score, err_count, warn_count] = result.get_summary();
     */
    [[nodiscard]] auto get_summary() const noexcept
        -> std::tuple<bool, double, size_t, size_t> {
        return {is_compliant, compliance_score, errors.size(), warnings.size()};
    }

private:
    void recalculate_score() {
        // Simple scoring: start at 100, subtract for each error/warning
        compliance_score = 100.0 - (static_cast<double>(errors.size()) * 10.0) - (static_cast<double>(warnings.size()) * 2.0);
        compliance_score = std::max(0.0, compliance_score);

        if (compliance_score < 100.0) {
            is_compliant = false;
        }
    }
};

} // namespace eti

// Qt Metatype declarations for signal/slot usage
// CRITICAL: All eti:: types used in Qt signals/slots MUST be registered here
Q_DECLARE_METATYPE(eti::EtiFrame)
Q_DECLARE_METATYPE(eti::Ensemble)
Q_DECLARE_METATYPE(eti::DabService)
Q_DECLARE_METATYPE(eti::ValidationResult)
Q_DECLARE_METATYPE(eti::SubChannelInfo)
Q_DECLARE_METATYPE(eti::ServiceComponent)
Q_DECLARE_METATYPE(eti::ServiceInfo)
Q_DECLARE_METATYPE(eti::FigBlock)

#endif // ETI_TYPES_HPP
