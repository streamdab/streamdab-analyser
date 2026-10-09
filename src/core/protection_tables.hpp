/**
 * @file protection_tables.hpp
 * @brief EEP/UEP sub-channel size tables (EN 300 401 §6.2) for reporting.
 *
 * Backlog item (8) of docs/fixes/CLI_FIG_COVERAGE.md (P2/P3): the CLI reports
 * the *protection table* value for every sub-channel — the canonical CU size
 * for the signalled protection — alongside the stream-signalled size:
 *   - EEP (long form, EN 300 401 Table 6.1 / etisnoop etianalyse.cpp STC-TPL
 *     section): option 0 (A) -> 16/8/6/4 CU, option 1 (B) -> 27/21/18/15 CU
 *     for protection levels 1..4.
 *   - UEP (short form, EN 300 401 Table 6.2): the 64-entry sub-channel size
 *     table plus the 0-based protection level per table index (values as
 *     published by ODR-DabMux MuxElements.h/cpp, which is derived from the
 *     standard's table).
 *
 * Header-only, no Qt dependency — usable from the core parser, the headless
 * trace/inventory walk and unit tests alike.
 *
 * @author C++ Qt Developer Agent (CLI FIG detail wave, v1.4)
 */

#pragma once

#include <cstdint>
#include <string>

namespace eti {

/**
 * @brief EEP sub-channel size in CUs per protection level (1-based 1..4),
 *        EN 300 401 Table 6.1. Index 0 = level 1.
 */
inline constexpr uint16_t kEepASizeCu[4] = {16, 8, 6, 4};    // EEP-1A..EEP-4A
inline constexpr uint16_t kEepBSizeCu[4] = {27, 21, 18, 15}; // EEP-1B..EEP-4B

/**
 * @brief UEP sub-channel size in CUs per 6-bit table index (0..63),
 *        EN 300 401 Table 6.2 (ODR-DabMux values, getSizeCu()).
 */
inline constexpr uint16_t kUepSizeCu[64] = {
    16, 21, 24, 29, 35, 24, 29, 35,
    42, 52, 29, 35, 42, 52, 32, 42,
    48, 58, 70, 40, 52, 58, 70, 84,
    48, 58, 70, 84, 104, 58, 70, 84,
    104, 64, 84, 96, 116, 140, 80, 104,
    116, 140, 168, 96, 116, 140, 168, 208,
    116, 140, 168, 208, 232, 128, 168, 192,
    232, 280, 160, 208, 280, 192, 280, 416
};

/**
 * @brief UEP protection level per table index, 0-based (0 = level 1).
 *
 * EN 300 401 §6.2 defines the protection level as a function of the table
 * index; ODR-DabMux encodes it in ProtectionLevelTable[] (ProtectionLevel =
 * table value, signalled 0-based in the STC TPL). 0-based so the STC path can
 * use it unchanged; add 1 for the 1-based level used in reports.
 */
inline constexpr uint8_t kUepLevel[64] = {
    4, 3, 2, 1, 0, 4, 3, 2, 1, 0, 4, 3, 2, 1, 4, 3,
    2, 1, 0, 4, 3, 2, 1, 0, 4, 3, 2, 1, 0, 4, 3, 2,
    1, 4, 3, 2, 1, 0, 4, 3, 2, 1, 0, 4, 3, 2, 1, 0,
    4, 3, 2, 1, 0, 4, 3, 2, 1, 0, 4, 3, 1, 4, 2, 0
};

/**
 * @brief EEP sub-channel size in CUs from the protection table.
 * @param option        Long-form option field (FIG 0/1 b6-b4): 0 = EEP-A,
 *                      1 = EEP-B; other values return 0 (unknown table).
 * @param level1Based   Protection level 1..4.
 * @return Table CU size, 0 when the option/level is out of table range.
 */
inline uint16_t eepSizeCu(uint8_t option, uint8_t level1Based)
{
    if (level1Based < 1 || level1Based > 4) {
        return 0;
    }
    switch (option) {
        case 0: return kEepASizeCu[level1Based - 1];
        case 1: return kEepBSizeCu[level1Based - 1];
        default: return 0;
    }
}

/**
 * @brief UEP sub-channel size in CUs from the protection table.
 * @param tableIndex  6-bit table index (0..63).
 * @return Table CU size (the table always covers the full 64-entry index
 *         space, so no out-of-range case exists for valid indices).
 */
inline uint16_t uepSizeCu(uint8_t tableIndex)
{
    return kUepSizeCu[tableIndex & 0x3F];
}

/**
 * @brief UEP protection level, 1-based (1..5), for a table index.
 */
inline uint8_t uepLevel1Based(uint8_t tableIndex)
{
    return static_cast<uint8_t>(kUepLevel[tableIndex & 0x3F] + 1);
}

/**
 * @brief Protection table name for reporting ("EEP-A", "EEP-B", "UEP-1"..
 *        "UEP-5"). Unknown EEP options return "EEP-?".
 * @param option       EEP long-form option (0 = A, 1 = B); ignored for UEP.
 * @param isUep        True for the short-form (UEP) convention.
 * @param tableIndex   UEP table index (used when isUep).
 */
inline std::string protectionTableName(uint8_t option, bool isUep,
                                       uint8_t tableIndex = 0)
{
    if (isUep) {
        return "UEP-" + std::to_string(uepLevel1Based(tableIndex));
    }
    switch (option) {
        case 0: return "EEP-A";
        case 1: return "EEP-B";
        default: return "EEP-?";
    }
}

} // namespace eti