/**
 * @file crc16.hpp
 * @brief Shared CRC-16/CCITT-FALSE helper (single implementation used by all
 *        FIB-CRC consumers in the analyser).
 *
 * Reference: ETSI EN 300 799 clause 5.2 / EN 300 401 clause 5.2 — the FIB
 * CRC is CRC-16/CCITT-FALSE (poly 0x1021, init 0xFFFF, no reflection,
 * no final XOR); the value transmitted in the last two bytes of a 32-byte
 * FIB is the one's complement, big-endian.
 *
 * @author C++ Qt Developer Agent (standards-review F6)
 */

#ifndef STREAMDAB_CRC16_HPP
#define STREAMDAB_CRC16_HPP

#include <cstddef>
#include <cstdint>

namespace eti {

/**
 * @brief Compute CRC-16/CCITT-FALSE over a byte buffer.
 * @param data Buffer start (may be nullptr when len == 0)
 * @param len  Number of bytes
 * @return     Raw CRC-16/CCITT-FALSE value (not complemented)
 */
inline uint16_t crc16ccitt_false(const uint8_t* data, size_t len)
{
    uint16_t crc = 0xFFFF;
    for (size_t i = 0; i < len; ++i) {
        crc ^= static_cast<uint16_t>(data[i]) << 8;
        for (int b = 0; b < 8; ++b) {
            if (crc & 0x8000) {
                crc = static_cast<uint16_t>((crc << 1) ^ 0x1021);
            } else {
                crc = static_cast<uint16_t>(crc << 1);
            }
        }
    }
    return crc;
}

} // namespace eti

#endif // STREAMDAB_CRC16_HPP