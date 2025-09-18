#pragma once

#include <QByteArray>
#include <cstdint>

/**
 * @brief Sample ETI frame data for testing
 * 
 * Provides realistic ETI frame samples for GUI testing and validation
 */
namespace TestFixtures {

/**
 * @brief Sample ETI frame header (24 bytes)
 */
const QByteArray SAMPLE_ETI_HEADER = QByteArray::fromHex(
    "E0E1"   // ERR + STAT fields  
    "FFFF"   // LIDATA (dummy)
    "0001"   // FC
    "0000"   // NST
    "00"     // FP
    "0000"   // MID
    "00"     // FL
    "000000" // Reserved
    "000000" // Reserved
    "000000" // Reserved
    "000000" // Reserved
    "0000"   // CRC (dummy)
);

/**
 * @brief Sample FIC data (32 bytes)
 */
const QByteArray SAMPLE_FIC_DATA = QByteArray::fromHex(
    "00001234" // FIG 0/0: Ensemble ID 0x1234
    "00"       // Flags
    "56"       // CIF count
    "FF"       // End marker
    "00000000000000000000000000000000" // Padding
    "00000000000000000000000000000000" // Padding
    "0000"     // CRC
);

/**
 * @brief Complete sample ETI frame
 */
const QByteArray SAMPLE_ETI_FRAME = SAMPLE_ETI_HEADER + SAMPLE_FIC_DATA;

/**
 * @brief Sample service data for constellation testing
 */
const QByteArray SAMPLE_SERVICE_DATA = QByteArray::fromHex(
    "ABCDABCDABCDABCDABCDABCDABCDABCD" // 32 bytes of sample audio data
    "1234567890ABCDEF1234567890ABCDEF" // Another 32 bytes
);

/**
 * @brief Get a sample ETI frame with specified service ID
 */
inline QByteArray getSampleEtiFrame(uint16_t serviceId = 0xABCD) {
    QByteArray frame = SAMPLE_ETI_FRAME;
    // Insert service ID at appropriate location
    frame[24] = (serviceId >> 8) & 0xFF;
    frame[25] = serviceId & 0xFF;
    return frame;
}

/**
 * @brief Get sample constellation data for visualization testing
 */
inline QByteArray getSampleConstellationData(int samples = 256) {
    QByteArray data;
    data.reserve(samples * 2); // I/Q samples
    
    for (int i = 0; i < samples; ++i) {
        // Generate simple constellation pattern
        int8_t i_val = static_cast<int8_t>(127 * std::cos(2.0 * M_PI * i / samples));
        int8_t q_val = static_cast<int8_t>(127 * std::sin(2.0 * M_PI * i / samples));
        data.append(i_val);
        data.append(q_val);
    }
    
    return data;
}

} // namespace TestFixtures