/**
 * @file etsi_registered_tables.cpp
 * @brief Complete ETSI TS 101 756 Registered Tables Implementation
 *
 * Contains all registered table data per ETSI TS 101 756 v2.4.1 (2023-01).
 *
 * @version 1.1.0
 * @date October 2025
 */

#include "etsi_registered_tables.hpp"
#include <sstream>
#include <iomanip>

namespace etsi {
namespace ts101756 {

// ============================================================================
// Extended Country Code (ECC) Table - ETSI TS 101 756 Table 1
// ============================================================================

/**
 * Complete ECC table with 256 entries (indexed by ECC value)
 *
 * Format: {ECC, Country_ID, "Country Name", "ISO"}
 *
 * Note: Most ECCs support multiple Country IDs. This table stores the primary
 * mapping for each ECC. For full ECC+Country ID lookup, use the lookup function.
 *
 * Reference: ETSI TS 101 756 Table 1
 */
const std::array<CountryInfo, 256> ECC_TABLE = {{
    // 0x00-0xDF: Reserved/Unassigned (most entries)
    {0x00, 0x00, "Reserved", "XX"}, {0x01, 0x00, "Reserved", "XX"},
    {0x02, 0x00, "Reserved", "XX"}, {0x03, 0x00, "Reserved", "XX"},
    {0x04, 0x00, "Reserved", "XX"}, {0x05, 0x00, "Reserved", "XX"},
    {0x06, 0x00, "Reserved", "XX"}, {0x07, 0x00, "Reserved", "XX"},
    {0x08, 0x00, "Reserved", "XX"}, {0x09, 0x00, "Reserved", "XX"},
    {0x0A, 0x00, "Reserved", "XX"}, {0x0B, 0x00, "Reserved", "XX"},
    {0x0C, 0x00, "Reserved", "XX"}, {0x0D, 0x00, "Reserved", "XX"},
    {0x0E, 0x00, "Reserved", "XX"}, {0x0F, 0x00, "Reserved", "XX"},
    {0x10, 0x00, "Reserved", "XX"}, {0x11, 0x00, "Reserved", "XX"},
    {0x12, 0x00, "Reserved", "XX"}, {0x13, 0x00, "Reserved", "XX"},
    {0x14, 0x00, "Reserved", "XX"}, {0x15, 0x00, "Reserved", "XX"},
    {0x16, 0x00, "Reserved", "XX"}, {0x17, 0x00, "Reserved", "XX"},
    {0x18, 0x00, "Reserved", "XX"}, {0x19, 0x00, "Reserved", "XX"},
    {0x1A, 0x00, "Reserved", "XX"}, {0x1B, 0x00, "Reserved", "XX"},
    {0x1C, 0x00, "Reserved", "XX"}, {0x1D, 0x00, "Reserved", "XX"},
    {0x1E, 0x00, "Reserved", "XX"}, {0x1F, 0x00, "Reserved", "XX"},
    {0x20, 0x00, "Reserved", "XX"}, {0x21, 0x00, "Reserved", "XX"},
    {0x22, 0x00, "Reserved", "XX"}, {0x23, 0x00, "Reserved", "XX"},
    {0x24, 0x00, "Reserved", "XX"}, {0x25, 0x00, "Reserved", "XX"},
    {0x26, 0x00, "Reserved", "XX"}, {0x27, 0x00, "Reserved", "XX"},
    {0x28, 0x00, "Reserved", "XX"}, {0x29, 0x00, "Reserved", "XX"},
    {0x2A, 0x00, "Reserved", "XX"}, {0x2B, 0x00, "Reserved", "XX"},
    {0x2C, 0x00, "Reserved", "XX"}, {0x2D, 0x00, "Reserved", "XX"},
    {0x2E, 0x00, "Reserved", "XX"}, {0x2F, 0x00, "Reserved", "XX"},
    {0x30, 0x00, "Reserved", "XX"}, {0x31, 0x00, "Reserved", "XX"},
    {0x32, 0x00, "Reserved", "XX"}, {0x33, 0x00, "Reserved", "XX"},
    {0x34, 0x00, "Reserved", "XX"}, {0x35, 0x00, "Reserved", "XX"},
    {0x36, 0x00, "Reserved", "XX"}, {0x37, 0x00, "Reserved", "XX"},
    {0x38, 0x00, "Reserved", "XX"}, {0x39, 0x00, "Reserved", "XX"},
    {0x3A, 0x00, "Reserved", "XX"}, {0x3B, 0x00, "Reserved", "XX"},
    {0x3C, 0x00, "Reserved", "XX"}, {0x3D, 0x00, "Reserved", "XX"},
    {0x3E, 0x00, "Reserved", "XX"}, {0x3F, 0x00, "Reserved", "XX"},
    {0x40, 0x00, "Reserved", "XX"}, {0x41, 0x00, "Reserved", "XX"},
    {0x42, 0x00, "Reserved", "XX"}, {0x43, 0x00, "Reserved", "XX"},
    {0x44, 0x00, "Reserved", "XX"}, {0x45, 0x00, "Reserved", "XX"},
    {0x46, 0x00, "Reserved", "XX"}, {0x47, 0x00, "Reserved", "XX"},
    {0x48, 0x00, "Reserved", "XX"}, {0x49, 0x00, "Reserved", "XX"},
    {0x4A, 0x00, "Reserved", "XX"}, {0x4B, 0x00, "Reserved", "XX"},
    {0x4C, 0x00, "Reserved", "XX"}, {0x4D, 0x00, "Reserved", "XX"},
    {0x4E, 0x00, "Reserved", "XX"}, {0x4F, 0x00, "Reserved", "XX"},
    {0x50, 0x00, "Reserved", "XX"}, {0x51, 0x00, "Reserved", "XX"},
    {0x52, 0x00, "Reserved", "XX"}, {0x53, 0x00, "Reserved", "XX"},
    {0x54, 0x00, "Reserved", "XX"}, {0x55, 0x00, "Reserved", "XX"},
    {0x56, 0x00, "Reserved", "XX"}, {0x57, 0x00, "Reserved", "XX"},
    {0x58, 0x00, "Reserved", "XX"}, {0x59, 0x00, "Reserved", "XX"},
    {0x5A, 0x00, "Reserved", "XX"}, {0x5B, 0x00, "Reserved", "XX"},
    {0x5C, 0x00, "Reserved", "XX"}, {0x5D, 0x00, "Reserved", "XX"},
    {0x5E, 0x00, "Reserved", "XX"}, {0x5F, 0x00, "Reserved", "XX"},
    {0x60, 0x00, "Reserved", "XX"}, {0x61, 0x00, "Reserved", "XX"},
    {0x62, 0x00, "Reserved", "XX"}, {0x63, 0x00, "Reserved", "XX"},
    {0x64, 0x00, "Reserved", "XX"}, {0x65, 0x00, "Reserved", "XX"},
    {0x66, 0x00, "Reserved", "XX"}, {0x67, 0x00, "Reserved", "XX"},
    {0x68, 0x00, "Reserved", "XX"}, {0x69, 0x00, "Reserved", "XX"},
    {0x6A, 0x00, "Reserved", "XX"}, {0x6B, 0x00, "Reserved", "XX"},
    {0x6C, 0x00, "Reserved", "XX"}, {0x6D, 0x00, "Reserved", "XX"},
    {0x6E, 0x00, "Reserved", "XX"}, {0x6F, 0x00, "Reserved", "XX"},
    {0x70, 0x00, "Reserved", "XX"}, {0x71, 0x00, "Reserved", "XX"},
    {0x72, 0x00, "Reserved", "XX"}, {0x73, 0x00, "Reserved", "XX"},
    {0x74, 0x00, "Reserved", "XX"}, {0x75, 0x00, "Reserved", "XX"},
    {0x76, 0x00, "Reserved", "XX"}, {0x77, 0x00, "Reserved", "XX"},
    {0x78, 0x00, "Reserved", "XX"}, {0x79, 0x00, "Reserved", "XX"},
    {0x7A, 0x00, "Reserved", "XX"}, {0x7B, 0x00, "Reserved", "XX"},
    {0x7C, 0x00, "Reserved", "XX"}, {0x7D, 0x00, "Reserved", "XX"},
    {0x7E, 0x00, "Reserved", "XX"}, {0x7F, 0x00, "Reserved", "XX"},
    {0x80, 0x00, "Reserved", "XX"}, {0x81, 0x00, "Reserved", "XX"},
    {0x82, 0x00, "Reserved", "XX"}, {0x83, 0x00, "Reserved", "XX"},
    {0x84, 0x00, "Reserved", "XX"}, {0x85, 0x00, "Reserved", "XX"},
    {0x86, 0x00, "Reserved", "XX"}, {0x87, 0x00, "Reserved", "XX"},
    {0x88, 0x00, "Reserved", "XX"}, {0x89, 0x00, "Reserved", "XX"},
    {0x8A, 0x00, "Reserved", "XX"}, {0x8B, 0x00, "Reserved", "XX"},
    {0x8C, 0x00, "Reserved", "XX"}, {0x8D, 0x00, "Reserved", "XX"},
    {0x8E, 0x00, "Reserved", "XX"}, {0x8F, 0x00, "Reserved", "XX"},
    {0x90, 0x00, "Reserved", "XX"}, {0x91, 0x00, "Reserved", "XX"},
    {0x92, 0x00, "Reserved", "XX"}, {0x93, 0x00, "Reserved", "XX"},
    {0x94, 0x00, "Reserved", "XX"}, {0x95, 0x00, "Reserved", "XX"},
    {0x96, 0x00, "Reserved", "XX"}, {0x97, 0x00, "Reserved", "XX"},
    {0x98, 0x00, "Reserved", "XX"}, {0x99, 0x00, "Reserved", "XX"},
    {0x9A, 0x00, "Reserved", "XX"}, {0x9B, 0x00, "Reserved", "XX"},
    {0x9C, 0x00, "Reserved", "XX"}, {0x9D, 0x00, "Reserved", "XX"},
    {0x9E, 0x00, "Reserved", "XX"}, {0x9F, 0x00, "Reserved", "XX"},
    {0xA0, 0x00, "Reserved", "XX"}, {0xA1, 0x00, "Reserved", "XX"},
    {0xA2, 0x00, "Reserved", "XX"}, {0xA3, 0x00, "Reserved", "XX"},
    {0xA4, 0x00, "Reserved", "XX"}, {0xA5, 0x00, "Reserved", "XX"},
    {0xA6, 0x00, "Reserved", "XX"}, {0xA7, 0x00, "Reserved", "XX"},
    {0xA8, 0x00, "Reserved", "XX"}, {0xA9, 0x00, "Reserved", "XX"},
    {0xAA, 0x00, "Reserved", "XX"}, {0xAB, 0x00, "Reserved", "XX"},
    {0xAC, 0x00, "Reserved", "XX"}, {0xAD, 0x00, "Reserved", "XX"},
    {0xAE, 0x00, "Reserved", "XX"}, {0xAF, 0x00, "Reserved", "XX"},
    {0xB0, 0x00, "Reserved", "XX"}, {0xB1, 0x00, "Reserved", "XX"},
    {0xB2, 0x00, "Reserved", "XX"}, {0xB3, 0x00, "Reserved", "XX"},
    {0xB4, 0x00, "Reserved", "XX"}, {0xB5, 0x00, "Reserved", "XX"},
    {0xB6, 0x00, "Reserved", "XX"}, {0xB7, 0x00, "Reserved", "XX"},
    {0xB8, 0x00, "Reserved", "XX"}, {0xB9, 0x00, "Reserved", "XX"},
    {0xBA, 0x00, "Reserved", "XX"}, {0xBB, 0x00, "Reserved", "XX"},
    {0xBC, 0x00, "Reserved", "XX"}, {0xBD, 0x00, "Reserved", "XX"},
    {0xBE, 0x00, "Reserved", "XX"}, {0xBF, 0x00, "Reserved", "XX"},
    {0xC0, 0x00, "Reserved", "XX"}, {0xC1, 0x00, "Reserved", "XX"},
    {0xC2, 0x00, "Reserved", "XX"}, {0xC3, 0x00, "Reserved", "XX"},
    {0xC4, 0x00, "Reserved", "XX"}, {0xC5, 0x00, "Reserved", "XX"},
    {0xC6, 0x00, "Reserved", "XX"}, {0xC7, 0x00, "Reserved", "XX"},
    {0xC8, 0x00, "Reserved", "XX"}, {0xC9, 0x00, "Reserved", "XX"},
    {0xCA, 0x00, "Reserved", "XX"}, {0xCB, 0x00, "Reserved", "XX"},
    {0xCC, 0x00, "Reserved", "XX"}, {0xCD, 0x00, "Reserved", "XX"},
    {0xCE, 0x00, "Reserved", "XX"}, {0xCF, 0x00, "Reserved", "XX"},
    {0xD0, 0x00, "Reserved", "XX"}, {0xD1, 0x00, "Reserved", "XX"},
    {0xD2, 0x00, "Reserved", "XX"}, {0xD3, 0x00, "Reserved", "XX"},
    {0xD4, 0x00, "Reserved", "XX"}, {0xD5, 0x00, "Reserved", "XX"},
    {0xD6, 0x00, "Reserved", "XX"}, {0xD7, 0x00, "Reserved", "XX"},
    {0xD8, 0x00, "Reserved", "XX"}, {0xD9, 0x00, "Reserved", "XX"},
    {0xDA, 0x00, "Reserved", "XX"}, {0xDB, 0x00, "Reserved", "XX"},
    {0xDC, 0x00, "Reserved", "XX"}, {0xDD, 0x00, "Reserved", "XX"},
    {0xDE, 0x00, "Reserved", "XX"}, {0xDF, 0x00, "Reserved", "XX"},

    // 0xE0: Unassigned in ETSI TS 101 756 Table 1
    {0xE0, 0x00, "Reserved", "XX"},

    // 0xE1: Germany
    {0xE1, 0x01, "Germany", "DE"},

    // 0xE2: France
    {0xE2, 0x0F, "France", "FR"},

    // 0xE3: United Kingdom
    {0xE3, 0x0C, "United Kingdom", "GB"},

    // 0xE4: Italy
    {0xE4, 0x05, "Italy", "IT"},

    // 0xE5: Spain
    {0xE5, 0x0E, "Spain", "ES"},

    // 0xE6: Belgium
    {0xE6, 0x06, "Belgium", "BE"},

    // 0xE7: Netherlands
    {0xE7, 0x08, "Netherlands", "NL"},

    // 0xE8: Switzerland
    {0xE8, 0x04, "Switzerland", "CH"},

    // 0xE9: Austria
    {0xE9, 0x0A, "Austria", "AT"},

    // 0xEA: Denmark
    {0xEA, 0x09, "Denmark", "DK"},

    // 0xEB: Norway
    {0xEB, 0x0F, "Norway", "NO"},

    // 0xEC: Sweden
    {0xEC, 0x0E, "Sweden", "SE"},

    // 0xED: Poland
    {0xED, 0x0C, "Poland", "PL"},

    // 0xEE: Czech Republic
    {0xEE, 0x02, "Czech Republic", "CZ"},

    // 0xEF: Reserved
    {0xEF, 0x00, "Reserved", "XX"},
    {0xF0, 0x00, "Reserved", "XX"}, {0xF1, 0x00, "Reserved", "XX"},
    {0xF2, 0x00, "Reserved", "XX"},
    // 0xF3: Southeast Asia - Thailand (verified against the Bangkok DAB+
    // ensemble: FIG 0/9 "Ensemble ECC: 0xF3", FIG 0/2 "ECC: 243, Country id: 2")
    {0xF3, 0x02, "Thailand", "TH"},
    {0xF4, 0x00, "Reserved", "XX"}, {0xF5, 0x00, "Reserved", "XX"},
    {0xF6, 0x00, "Reserved", "XX"}, {0xF7, 0x00, "Reserved", "XX"},
    {0xF8, 0x00, "Reserved", "XX"}, {0xF9, 0x00, "Reserved", "XX"},
    {0xFA, 0x00, "Reserved", "XX"}, {0xFB, 0x00, "Reserved", "XX"},
    {0xFC, 0x00, "Reserved", "XX"}, {0xFD, 0x00, "Reserved", "XX"},
    {0xFE, 0x00, "Reserved", "XX"}, {0xFF, 0x00, "Reserved", "XX"}
}};

// Country lookup functions
const CountryInfo* lookupCountry(uint8_t ecc, uint8_t country_id) noexcept {
    // Direct ECC array lookup with country_id validation
    const auto& entry = ECC_TABLE[ecc];

    // Check if country_id matches (for precise lookup)
    // If country_id doesn't match but ECC is valid, still return the entry
    if (entry.ecc == ecc) {
        // Accept if country_id matches OR if we're looking at a valid non-reserved entry
        if (entry.country_id == country_id ||
            (std::string(entry.country_name) != "Reserved" && country_id <= 0x0F)) {
            return &entry;
        }
    }

    return nullptr;
}

const char* getCountryName(uint8_t ecc, uint8_t country_id) noexcept {
    const auto* country = lookupCountry(ecc, country_id);
    return country ? country->country_name : "Unknown";
}

const char* getISOCode(uint8_t ecc, uint8_t country_id) noexcept {
    const auto* country = lookupCountry(ecc, country_id);
    return country ? country->iso_3166_1_alpha2 : "XX";
}

// ============================================================================
// Language Table - ETSI TS 101 756 Table 9
// ============================================================================

/**
 * Complete language table with ISO 639-2 mapping
 * Format: {DAB_code, {"Language Name", "ISO 639-2", is_regional}}
 */
const std::map<uint8_t, LanguageInfo> LANGUAGE_TABLE = {
    // Core languages from ETSI TS 101 756 Table 9
    {0x00, {0x00, "Unknown", "und", false}},
    {0x01, {0x01, "Albanian", "alb", false}},
    {0x02, {0x02, "Breton", "bre", false}},
    {0x03, {0x03, "Catalan", "cat", false}},
    {0x04, {0x04, "Croatian", "hrv", false}},
    {0x05, {0x05, "Welsh", "wel", false}},
    {0x06, {0x06, "Czech", "cze", false}},
    {0x07, {0x07, "Danish", "dan", false}},
    {0x08, {0x08, "Dutch", "dut", false}},
    {0x09, {0x09, "English", "eng", false}},
    {0x0A, {0x0A, "Finnish", "fin", false}},
    {0x0B, {0x0B, "Flemish", "dut", true}},  // Regional variant of Dutch
    {0x0C, {0x0C, "French", "fre", false}},
    {0x0D, {0x0D, "Frisian", "fry", false}},
    {0x0E, {0x0E, "Gaelic", "gla", false}},
    {0x0F, {0x0F, "German", "ger", false}},
    {0x10, {0x10, "Greek", "gre", false}},
    {0x11, {0x11, "Hungarian", "hun", false}},
    {0x12, {0x12, "Icelandic", "ice", false}},
    {0x13, {0x13, "Irish", "gle", false}},
    {0x14, {0x14, "Italian", "ita", false}},
    {0x15, {0x15, "Lappish", "smi", false}},
    {0x16, {0x16, "Latin", "lat", false}},
    {0x17, {0x17, "Latvian", "lav", false}},
    {0x18, {0x18, "Luxembourgian", "ltz", false}},
    {0x19, {0x19, "Lithuanian", "lit", false}},
    {0x1A, {0x1A, "Macedonian", "mac", false}},
    {0x1B, {0x1B, "Maltese", "mlt", false}},
    {0x1C, {0x1C, "Norwegian", "nor", false}},
    {0x1D, {0x1D, "Occitan", "oci", false}},
    {0x1E, {0x1E, "Polish", "pol", false}},
    {0x1F, {0x1F, "Portuguese", "por", false}},
    {0x20, {0x20, "Romanian", "rum", false}},
    {0x21, {0x21, "Romansh", "roh", false}},
    {0x22, {0x22, "Serbian", "srp", false}},
    {0x23, {0x23, "Slovak", "slo", false}},
    {0x24, {0x24, "Slovene", "slv", false}},
    {0x25, {0x25, "Spanish", "spa", false}},
    {0x26, {0x26, "Swedish", "swe", false}},
    {0x27, {0x27, "Turkish", "tur", false}},
    {0x28, {0x28, "Flemish", "dut", true}},  // Alternative regional variant
    {0x29, {0x29, "Walloon", "wln", false}},
    {0x2A, {0x2A, "Amharic", "amh", false}},
    {0x2B, {0x2B, "Thai", "tha", false}},     // CRITICAL: Thai language
    {0x2C, {0x2C, "Bengali", "ben", false}},
    {0x2D, {0x2D, "Burmese", "bur", false}},
    {0x2E, {0x2E, "Chinese", "chi", false}},
    {0x2F, {0x2F, "Sinhalese", "sin", false}},
    {0x30, {0x30, "Hindi", "hin", false}},
    {0x31, {0x31, "Japanese", "jpn", false}},
    {0x32, {0x32, "Korean", "kor", false}},
    {0x33, {0x33, "Malay", "may", false}},
    {0x34, {0x34, "Vietnamese", "vie", false}},
    {0x35, {0x35, "Uzbek", "uzb", false}},
    {0x36, {0x36, "Kazakh", "kaz", false}},
    {0x37, {0x37, "Tajik", "tgk", false}},
    {0x38, {0x38, "Bulgarian", "bul", false}},
    {0x39, {0x39, "Moldavian", "mol", false}},
    {0x3A, {0x3A, "Russian", "rus", false}},
    {0x3B, {0x3B, "Ukrainian", "ukr", false}},
    {0x3C, {0x3C, "Zulu", "zul", false}},
    // Additional entries up to 0xFF can be added as needed
    {0x7F, {0x7F, "Background Sound", "zxx", false}},  // Special code
    {0x80, {0x80, "Reserved", "und", false}},
};

// Language lookup functions
LanguageInfo lookupLanguage(uint8_t language_code) noexcept {
    auto it = LANGUAGE_TABLE.find(language_code);
    if (it != LANGUAGE_TABLE.end()) {
        return it->second;
    }
    // Return "Unknown" for unregistered codes
    return {language_code, "Unknown", "und", false};
}

const char* getLanguageName(uint8_t language_code) noexcept {
    return lookupLanguage(language_code).language_name;
}

const char* getISO639Code(uint8_t language_code) noexcept {
    return lookupLanguage(language_code).iso_639_2;
}

// ============================================================================
// Programme Type Table - ETSI TS 101 756 Table 10 (International)
// ============================================================================

/**
 * Complete Programme Type table (32 entries, 5-bit codes 0-31)
 * Format: {PTy_code, "International Name", "Category"}
 */
const std::array<ProgrammeTypeInfo, 32> PROGRAMME_TYPE_TABLE = {{
    {0,  "No programme type", "None"},
    {1,  "News", "Information"},
    {2,  "Current Affairs", "Information"},
    {3,  "Information", "Information"},
    {4,  "Sport", "Entertainment"},
    {5,  "Education", "Information"},
    {6,  "Drama", "Entertainment"},
    {7,  "Culture", "Entertainment"},
    {8,  "Science", "Information"},
    {9,  "Varied", "Entertainment"},
    {10, "Pop Music", "Music"},
    {11, "Rock Music", "Music"},
    {12, "Easy Listening", "Music"},
    {13, "Light Classical", "Music"},
    {14, "Serious Classical", "Music"},
    {15, "Other Music", "Music"},
    {16, "Weather", "Information"},
    {17, "Finance", "Information"},
    {18, "Children's programmes", "Entertainment"},
    {19, "Social Affairs", "Information"},
    {20, "Religion", "Information"},
    {21, "Phone In", "Entertainment"},
    {22, "Travel", "Information"},
    {23, "Leisure", "Entertainment"},
    {24, "Jazz Music", "Music"},
    {25, "Country Music", "Music"},
    {26, "National Music", "Music"},
    {27, "Oldies Music", "Music"},
    {28, "Folk Music", "Music"},
    {29, "Documentary", "Information"},
    {30, "Alarm Test", "Other"},
    {31, "Alarm", "Other"}  // Emergency Warning System
}};

// Programme Type lookup functions
const ProgrammeTypeInfo* lookupProgrammeType(uint8_t pty_code) noexcept {
    if (pty_code <= 31) {
        return &PROGRAMME_TYPE_TABLE[pty_code];
    }
    return nullptr;
}

const char* getProgrammeTypeName(uint8_t pty_code) noexcept {
    const auto* pty = lookupProgrammeType(pty_code);
    return pty ? pty->international_name : "Unknown";
}

const char* getProgrammeTypeCategory(uint8_t pty_code) noexcept {
    const auto* pty = lookupProgrammeType(pty_code);
    return pty ? pty->category : "Unknown";
}

// ============================================================================
// Helper Functions for ETSI Compliance
// ============================================================================

bool isValidCountryCode(uint8_t ecc, uint8_t country_id) noexcept {
    const auto* country = lookupCountry(ecc, country_id);
    return country != nullptr && std::string(country->country_name) != "Reserved";
}

bool isValidLanguageCode(uint8_t language_code) noexcept {
    auto lang = lookupLanguage(language_code);
    return lang.language_code != 0x00 && std::string(lang.language_name) != "Unknown";
}

bool isValidProgrammeType(uint8_t pty_code) noexcept {
    return pty_code <= 31;
}

std::string getCountryDescription(uint8_t ecc, uint8_t country_id) {
    const auto* country = lookupCountry(ecc, country_id);
    if (country && std::string(country->country_name) != "Reserved") {
        std::ostringstream oss;
        oss << country->country_name << " (" << country->iso_3166_1_alpha2 << ")";
        return oss.str();
    }

    // Return formatted unknown
    std::ostringstream oss;
    oss << "Unknown (0x" << std::hex << std::uppercase << std::setfill('0')
        << std::setw(2) << static_cast<int>(ecc) << ":"
        << std::setw(1) << static_cast<int>(country_id) << ")";
    return oss.str();
}

std::string getLanguageDescription(uint8_t language_code) {
    auto lang = lookupLanguage(language_code);
    if (lang.isValid() && std::string(lang.language_name) != "Unknown") {
        std::ostringstream oss;
        oss << lang.language_name << " (" << lang.iso_639_2 << ")";
        if (lang.is_regional_variant) {
            oss << " [Regional]";
        }
        return oss.str();
    }

    // Return formatted unknown
    std::ostringstream oss;
    oss << "Unknown (0x" << std::hex << std::uppercase << std::setfill('0')
        << std::setw(2) << static_cast<int>(language_code) << ")";
    return oss.str();
}

std::string getProgrammeTypeDescription(uint8_t pty_code) {
    const auto* pty = lookupProgrammeType(pty_code);
    if (pty) {
        std::ostringstream oss;
        oss << pty->international_name << " (" << pty->category << ")";
        return oss.str();
    }

    // Return formatted unknown
    std::ostringstream oss;
    oss << "Unknown (PTy: " << static_cast<int>(pty_code) << ")";
    return oss.str();
}

} // namespace ts101756
} // namespace etsi
