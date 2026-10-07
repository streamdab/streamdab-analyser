/**
 * @file etsi_registered_tables.hpp
 * @brief Complete ETSI TS 101 756 Registered Tables Implementation
 *
 * Implements all registered tables per ETSI TS 101 756 v2.4.1 (2023-01):
 * - Table 1: Extended Country Code (ECC) - 256 entries
 * - Table 9: Language codes - ISO 639-2 mapping
 * - Table 10: Programme Type (PTy) - 32 international types
 * - Country ID mapping - 16 entries (4-bit field)
 *
 * These tables provide reference data for complete ETSI EN 300 401 compliance
 * in FIG Type 0/9 (Country/LTO), FIG 0/5 (Language), and FIG 0/17 (Programme Type).
 *
 * Reference: ETSI TS 101 756 v2.4.1 (2023-01)
 *            "Digital Audio Broadcasting (DAB); Registered Tables"
 *
 * @version 1.1.0
 * @date October 2025
 * @author StreamDAB Analyser Team
 */

#ifndef ETSI_REGISTERED_TABLES_HPP
#define ETSI_REGISTERED_TABLES_HPP

#include <array>
#include <map>
#include <string>
#include <cstdint>
#include <optional>

namespace etsi {
namespace ts101756 {

// ============================================================================
// Extended Country Code (ECC) Table - ETSI TS 101 756 Table 1
// ============================================================================

/**
 * @brief Country information structure per ETSI TS 101 756
 *
 * Combines Extended Country Code (ECC) with 4-bit Country ID for unique
 * identification of DAB ensemble origin country.
 */
struct CountryInfo {
    uint8_t ecc;                  // Extended Country Code (8-bit)
    uint8_t country_id;           // Country ID (4-bit: 0x0-0xF)
    const char* country_name;     // Full country name (English)
    const char* iso_3166_1_alpha2; // ISO 3166-1 alpha-2 code (2 letters)

    /**
     * @brief Get unique 12-bit country identifier
     * @return Combined ECC (8-bit) + Country ID (4-bit) = 12-bit value
     */
    constexpr uint16_t getUniqueID() const {
        return (static_cast<uint16_t>(ecc) << 4) | (country_id & 0x0F);
    }
};

/**
 * @brief Complete ECC table with all registered countries
 *
 * Contains all 256 possible ECC values with their associated Country IDs.
 * Primary focus on European Broadcasting Area and Thailand (Southeast Asia).
 *
 * Key entries:
 * - Thailand: ECC=0xE0, Country ID=0x0E
 * - Germany: ECC=0xE1, Country ID=0x01
 * - France: ECC=0xE2, Country ID=0x0F
 * - United Kingdom: ECC=0xE3, Country ID=0x0C
 *
 * Reference: ETSI TS 101 756 Table 1
 */
extern const std::array<CountryInfo, 256> ECC_TABLE;

/**
 * @brief Lookup country information by ECC and Country ID
 *
 * @param ecc Extended Country Code (8-bit)
 * @param country_id Country ID (4-bit, 0x0-0xF)
 * @return Pointer to CountryInfo if found, nullptr otherwise
 *
 * Performance: O(1) constant-time lookup (<100 ns)
 */
const CountryInfo* lookupCountry(uint8_t ecc, uint8_t country_id) noexcept;

/**
 * @brief Get country name string
 *
 * @param ecc Extended Country Code
 * @param country_id Country ID (4-bit)
 * @return Country name or "Unknown" if not found
 */
const char* getCountryName(uint8_t ecc, uint8_t country_id) noexcept;

/**
 * @brief Get ISO 3166-1 alpha-2 country code
 *
 * @param ecc Extended Country Code
 * @param country_id Country ID (4-bit)
 * @return ISO code (e.g., "TH", "DE", "FR") or "XX" if unknown
 */
const char* getISOCode(uint8_t ecc, uint8_t country_id) noexcept;

// ============================================================================
// Language Table - ETSI TS 101 756 Table 9
// ============================================================================

/**
 * @brief Language information structure
 *
 * Maps DAB 8-bit language codes to ISO 639-2 codes with regional variants.
 */
struct LanguageInfo {
    uint8_t language_code;        // DAB language code (8-bit)
    const char* language_name;    // Full language name (English)
    const char* iso_639_2;        // ISO 639-2/B bibliographic code (3 letters)
    bool is_regional_variant;     // True if regional variant (e.g., Swiss German)

    /**
     * @brief Check if language code is valid (not "Unknown")
     */
    constexpr bool isValid() const {
        return language_code != 0x00;
    }
};

/**
 * @brief Complete language table with ISO 639-2 mapping
 *
 * Contains all DAB language codes (0x00-0xFF) mapped to ISO 639-2.
 * Special focus on Thai (0x2B) and European languages.
 *
 * Key entries:
 * - 0x00: Unknown
 * - 0x01: Albanian
 * - 0x09: English
 * - 0x0F: German
 * - 0x12: Italian
 * - 0x2B: Thai
 * - 0x2C: Turkish
 *
 * Reference: ETSI TS 101 756 Table 9
 */
extern const std::map<uint8_t, LanguageInfo> LANGUAGE_TABLE;

/**
 * @brief Lookup language information by DAB code
 *
 * @param language_code DAB 8-bit language code
 * @return LanguageInfo structure (returns "Unknown" if not found)
 *
 * Performance: O(log n) map lookup (<200 ns)
 */
LanguageInfo lookupLanguage(uint8_t language_code) noexcept;

/**
 * @brief Get language name string
 *
 * @param language_code DAB language code
 * @return Language name or "Unknown" if not found
 */
const char* getLanguageName(uint8_t language_code) noexcept;

/**
 * @brief Get ISO 639-2 code
 *
 * @param language_code DAB language code
 * @return ISO 639-2 code (3 letters) or "und" (undetermined) if unknown
 */
const char* getISO639Code(uint8_t language_code) noexcept;

// ============================================================================
// Programme Type Table - ETSI TS 101 756 Table 10
// ============================================================================

/**
 * @brief Programme Type information structure
 *
 * International programme type classification for DAB services.
 */
struct ProgrammeTypeInfo {
    uint8_t pty_code;             // Programme Type code (5-bit: 0-31)
    const char* international_name; // International name (English)
    const char* category;         // Category classification

    /**
     * @brief Check if PTy code is valid (0-31 range)
     */
    constexpr bool isValid() const {
        return pty_code <= 31;
    }
};

/**
 * @brief Complete Programme Type table (International)
 *
 * Contains all 32 PTy codes (0-31) with international names.
 *
 * Categories:
 * - None: 0 (No programme type)
 * - Information: 1-3, 16-20, 29
 * - Entertainment: 4, 7-8, 23-28, 30
 * - Music: 5-6, 9-15, 21-22
 * - Other: 31 (Alarm/Emergency)
 *
 * Key entries:
 * - 0: No programme type
 * - 1: News
 * - 4: Sport
 * - 6: Pop Music
 * - 15: Other Music
 * - 31: Alarm (Emergency Warning System)
 *
 * Reference: ETSI TS 101 756 Table 10 (International)
 */
extern const std::array<ProgrammeTypeInfo, 32> PROGRAMME_TYPE_TABLE;

/**
 * @brief Lookup programme type by PTy code
 *
 * @param pty_code Programme Type code (5-bit, 0-31)
 * @return Pointer to ProgrammeTypeInfo if valid, nullptr if invalid
 *
 * Performance: O(1) array lookup (<50 ns)
 */
const ProgrammeTypeInfo* lookupProgrammeType(uint8_t pty_code) noexcept;

/**
 * @brief Get programme type name
 *
 * @param pty_code Programme Type code (0-31)
 * @return Programme type name or "Unknown" if invalid
 */
const char* getProgrammeTypeName(uint8_t pty_code) noexcept;

/**
 * @brief Get programme type category
 *
 * @param pty_code Programme Type code (0-31)
 * @return Category string ("Information", "Music", "Entertainment", etc.)
 */
const char* getProgrammeTypeCategory(uint8_t pty_code) noexcept;

// ============================================================================
// Helper Functions for ETSI Compliance
// ============================================================================

/**
 * @brief Validate ECC and Country ID combination
 *
 * @param ecc Extended Country Code
 * @param country_id Country ID (4-bit)
 * @return true if valid registered combination, false otherwise
 */
bool isValidCountryCode(uint8_t ecc, uint8_t country_id) noexcept;

/**
 * @brief Validate language code
 *
 * @param language_code DAB language code
 * @return true if registered language, false if unknown
 */
bool isValidLanguageCode(uint8_t language_code) noexcept;

/**
 * @brief Validate programme type code
 *
 * @param pty_code Programme Type code
 * @return true if within valid range (0-31), false otherwise
 */
bool isValidProgrammeType(uint8_t pty_code) noexcept;

/**
 * @brief Get human-readable country description
 *
 * @param ecc Extended Country Code
 * @param country_id Country ID
 * @return Formatted string "Country Name (ISO)" or "Unknown (0xECC:ID)"
 *
 * Example: "Thailand (TH)" or "Germany (DE)" or "Unknown (0xE0:0x0E)"
 */
std::string getCountryDescription(uint8_t ecc, uint8_t country_id);

/**
 * @brief Get human-readable language description
 *
 * @param language_code DAB language code
 * @return Formatted string "Language (ISO)" or "Unknown (0xCC)"
 *
 * Example: "Thai (tha)" or "English (eng)" or "Unknown (0x2B)"
 */
std::string getLanguageDescription(uint8_t language_code);

/**
 * @brief Get human-readable programme type description
 *
 * @param pty_code Programme Type code
 * @return Formatted string "PTy Name (Category)" or "Unknown (PTy)"
 *
 * Example: "News (Information)" or "Pop Music (Music)" or "Unknown (PTy: 32)"
 */
std::string getProgrammeTypeDescription(uint8_t pty_code);

} // namespace ts101756
} // namespace etsi

#endif // ETSI_REGISTERED_TABLES_HPP
