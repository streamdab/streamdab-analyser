/**
 * @file test_utils.h
 * @brief Shared Test Utilities for Integration Tests
 *
 * Provides synthetic ETI frame generation and test data creation
 * for audio and MOT pipeline integration tests.
 *
 * Features:
 * - Minimal ETI frame generation (6144 bytes)
 * - ETI frames with audio data
 * - ETI frames with data services
 * - MOT data group generation
 * - FIG data generation for testing
 *
 * Phase 3A Wave 3.1: Integration & Testing Infrastructure
 *
 * @author StreamDAB Development Team
 * @date October 22, 2025
 */

#pragma once

#include <QByteArray>
#include <QString>
#include <vector>
#include <cstdint>
#include <array>

namespace TestUtils {

/**
 * @brief Generate minimal valid ETI frame (6144 bytes)
 *
 * Creates a basic ETI frame with:
 * - Valid sync pattern (0xFF1F491F)
 * - Frame Count (FCT) = 0
 * - Mode I
 * - Minimal FIC data
 * - Minimal MSC data
 *
 * @return 6144-byte ETI frame
 */
inline QByteArray generateMinimalETIFrame() {
    QByteArray frame(6144, 0x00);
    
    // Set sync pattern (ERR=0, FSYNC=0x073AB6, LIDATA follows)
    frame[0] = 0xFF;
    frame[1] = 0x1F;
    frame[2] = 0x49;
    frame[3] = 0x1F;
    
    // Frame Count (FCT) - offset 4, bits 0-7
    frame[4] = 0x00;  // FCT = 0
    
    // FICF (FIC present) = 1, NST = 1 (1 stream), FP = 000
    // FC (Frame Count) high bits
    frame[5] = 0x21;  // FICF=1, NST=1
    
    // MID (Mode ID) = 01 (Mode I), FL (Frame Length) = 3 FIBs
    frame[6] = 0x40;  // MID=01 (Mode I)
    
    // FIC data starts at offset 8 (after LIDATA)
    // For Mode I: 3 FIBs × 32 bytes = 96 bytes
    // Fill with minimal FIG data
    
    // FIB 0: FIG Type 0/0 (Ensemble information)
    int fic_offset = 8;
    frame[fic_offset] = 0x00;  // FIG header: Type 0, length 0
    
    // Fill rest of FIC with padding (0xFF)
    for (int i = fic_offset + 1; i < fic_offset + 96; i++) {
        frame[i] = 0xFF;
    }
    
    // MSC data follows FIC (rest of frame)
    // Fill with zeros (minimal audio data)
    
    return frame;
}

/**
 * @brief Generate ETI frame with audio data
 *
 * Creates ETI frame with:
 * - Valid sync and header
 * - FIC with service information
 * - MSC with synthetic audio superframe structure
 *
 * @return 6144-byte ETI frame with audio data
 */
inline QByteArray generateETIFrameWithAudioData() {
    QByteArray frame = generateMinimalETIFrame();
    
    // Add audio-specific FIC data
    int fic_offset = 8;
    
    // FIG Type 0/1: Service organization (basic subchannel info)
    frame[fic_offset] = 0x01;      // FIG Type 0/1
    frame[fic_offset + 1] = 0x10;  // Length = 16 bytes
    frame[fic_offset + 2] = 0x00;  // C/N, OE, PD flags
    
    // Service component info
    frame[fic_offset + 3] = 0x00;  // SubChId = 0
    frame[fic_offset + 4] = 0x01;  // Audio service
    
    // MSC audio superframe structure (simplified)
    int msc_offset = 8 + 96;  // After FIC
    
    // Audio superframe header (DAB+)
    // FireCode, num_aus, SBR flag, PS flag, DAC rate
    frame[msc_offset] = 0x12;      // FireCode high byte
    frame[msc_offset + 1] = 0x34;  // FireCode low byte
    frame[msc_offset + 2] = 0x04;  // num_aus = 4
    frame[msc_offset + 3] = 0x03;  // SBR=1, PS=1, DAC_rate=0 (48kHz)
    
    return frame;
}

/**
 * @brief Generate ETI frame with specific service
 *
 * Creates ETI frame with:
 * - Valid sync and header
 * - FIC containing specified service ID and label
 * - MSC data
 *
 * @param service_id DAB service ID (SID)
 * @param label Service label (max 16 characters)
 * @return 6144-byte ETI frame with service information
 */
inline QByteArray generateETIFrameWithService(uint32_t service_id, const QString& label) {
    QByteArray frame = generateMinimalETIFrame();
    
    int fic_offset = 8;
    
    // FIG Type 1/1: Service label
    frame[fic_offset] = 0x20;      // FIG Type 1/1 (001 << 5)
    frame[fic_offset + 1] = 0x15;  // Length = 21 bytes
    frame[fic_offset + 2] = 0x00;  // C/N, OE, PD flags
    
    // Service ID (16 bits for Programme service)
    frame[fic_offset + 3] = (service_id >> 8) & 0xFF;
    frame[fic_offset + 4] = service_id & 0xFF;
    
    // Service label (16 bytes, padded with spaces)
    QByteArray label_bytes = label.toUtf8();
    for (int i = 0; i < 16; i++) {
        if (i < label_bytes.size()) {
            frame[fic_offset + 5 + i] = label_bytes[i];
        } else {
            frame[fic_offset + 5 + i] = 0x20;  // Space padding
        }
    }
    
    // Character flag field (2 bytes) - indicates UTF-8
    frame[fic_offset + 21] = 0x00;
    frame[fic_offset + 22] = 0x00;
    
    return frame;
}

/**
 * @brief Generate ETI frame with data service
 *
 * Creates ETI frame with:
 * - Valid sync and header
 * - FIC with data service component information
 * - MSC with data channel
 *
 * @return 6144-byte ETI frame with data service
 */
inline QByteArray generateETIFrameWithDataService() {
    QByteArray frame = generateMinimalETIFrame();
    
    int fic_offset = 8;
    
    // FIG Type 0/2: Service component (data service)
    frame[fic_offset] = 0x02;      // FIG Type 0/2
    frame[fic_offset + 1] = 0x08;  // Length = 8 bytes
    frame[fic_offset + 2] = 0x01;  // PD=1 (data service)
    
    // Service component info
    frame[fic_offset + 3] = 0xE1;  // Service ID high
    frame[fic_offset + 4] = 0xC0;  // Service ID mid
    frame[fic_offset + 5] = 0x03;  // Service ID low
    frame[fic_offset + 6] = 0x79;  // Service ID lowest
    
    frame[fic_offset + 7] = 0x00;  // SCIdS
    frame[fic_offset + 8] = 0x01;  // SubChId = 1
    frame[fic_offset + 9] = 0x3F;  // TMId = MSC data
    frame[fic_offset + 10] = 0x00; // DSCTy (MOT)
    
    return frame;
}

/**
 * @brief Generate MOT Data Group
 *
 * Creates a minimal MOT data group with:
 * - Data group header
 * - Segment header
 * - Transport ID
 * - Segment data
 *
 * @param transport_id MOT transport ID (0-1023)
 * @return Data group as QByteArray
 */
inline QByteArray generateMOTDataGroup(uint16_t transport_id) {
    std::vector<uint8_t> data_group;
    
    // Data Group Header (ETSI EN 300 401 Section 5.3.3.1)
    // Extension flag, CRC flag, Segment flag, User access flag
    data_group.push_back(0x50);  // Extension=0, CRC=1, Segment=1, User=0
    
    // Data group type (MSC data group type 3 = MOT)
    data_group.push_back(0x03);
    
    // Continuity index (4 bits) + Repetition index (4 bits)
    data_group.push_back(0x00);
    
    // Extension field (if present) - none here
    
    // Segment header (ETSI EN 301 234 Section 4.1)
    // Repetition count (3 bits) + Segment size (13 bits)
    uint16_t segment_size = 64;  // 64 bytes of segment data
    data_group.push_back((segment_size >> 8) & 0x1F);
    data_group.push_back(segment_size & 0xFF);
    
    // Segment number (16 bits) - segment 0
    data_group.push_back(0x00);
    data_group.push_back(0x00);
    
    // Transport ID (10 bits) + ...
    data_group.push_back((transport_id >> 2) & 0xFF);
    data_group.push_back((transport_id & 0x03) << 6);
    
    // Segment data (dummy data)
    for (int i = 0; i < segment_size; i++) {
        data_group.push_back(0xAA);
    }
    
    // CRC-16 (simplified - would need proper calculation)
    data_group.push_back(0x12);
    data_group.push_back(0x34);
    
    return QByteArray(reinterpret_cast<const char*>(data_group.data()),
                      static_cast<int>(data_group.size()));
}

/**
 * @brief Generate complete MOT object (minimal)
 *
 * Creates a minimal complete MOT object with:
 * - MOT header
 * - Body data
 *
 * @return Complete MOT object as QByteArray
 */
inline QByteArray generateCompleteMOTObject() {
    std::vector<uint8_t> mot_object;
    
    // MOT Header (ETSI EN 301 234 Section 5.1)
    // Body size (13 bits) = 256 bytes
    uint16_t body_size = 256;
    mot_object.push_back((body_size >> 5) & 0xFF);
    mot_object.push_back((body_size & 0x1F) << 3);
    
    // Header size (13 bits) = 32 bytes
    uint16_t header_size = 32;
    mot_object.push_back((header_size >> 8) & 0x1F);
    mot_object.push_back(header_size & 0xFF);
    
    // Content type (6 bits) = JPEG (0x02)
    mot_object.push_back(0x02 << 2);
    
    // Content subtype (9 bits) = 0
    mot_object.push_back(0x00);
    mot_object.push_back(0x00);
    
    // Padding to header size
    while (mot_object.size() < header_size) {
        mot_object.push_back(0x00);
    }
    
    // Body data (synthetic JPEG-like data)
    // JPEG starts with FFD8
    mot_object.push_back(0xFF);
    mot_object.push_back(0xD8);
    
    // Fill rest of body
    for (size_t i = 2; i < body_size; i++) {
        mot_object.push_back(0xAA);
    }
    
    return QByteArray(reinterpret_cast<const char*>(mot_object.data()),
                      static_cast<int>(mot_object.size()));
}

/**
 * @brief Generate FIG Type 0/0 data (Ensemble information)
 *
 * @param ensemble_id Ensemble ID
 * @param change_flag Change flag
 * @return FIG 0/0 data
 */
inline std::vector<uint8_t> generateFIG0_0(uint16_t ensemble_id, bool change_flag) {
    std::vector<uint8_t> fig_data;
    
    // FIG header: Type=0, Length=4, C/N=0, OE=0, PD=0
    fig_data.push_back(0x00);
    fig_data.push_back(0x04);
    fig_data.push_back(0x00);
    
    // Ensemble ID (16 bits)
    fig_data.push_back((ensemble_id >> 8) & 0xFF);
    fig_data.push_back(ensemble_id & 0xFF);
    
    // Change flags (16 bits)
    fig_data.push_back(change_flag ? 0x80 : 0x00);
    fig_data.push_back(0x00);
    
    // Alarm flag
    fig_data.push_back(0x00);
    
    return fig_data;
}

/**
 * @brief Calculate CRC-16 CCITT (for data groups)
 *
 * @param data Input data
 * @return 16-bit CRC
 */
inline uint16_t calculateCRC16(const std::vector<uint8_t>& data) {
    uint16_t crc = 0xFFFF;
    const uint16_t polynomial = 0x1021;
    
    for (uint8_t byte : data) {
        crc ^= (byte << 8);
        for (int i = 0; i < 8; i++) {
            if (crc & 0x8000) {
                crc = (crc << 1) ^ polynomial;
            } else {
                crc <<= 1;
            }
        }
    }
    
    return crc;
}

/**
 * @brief Verify ETI frame structure
 *
 * @param frame ETI frame data
 * @return true if basic structure is valid
 */
inline bool verifyETIFrameStructure(const QByteArray& frame) {
    if (frame.size() != 6144) {
        return false;
    }
    
    // Check sync pattern (first 4 bytes)
    const uint8_t* data = reinterpret_cast<const uint8_t*>(frame.constData());
    
    // Valid sync patterns: 0xFF1F491F, 0xFF1FC4FF
    if (data[0] == 0xFF && data[1] == 0x1F) {
        if ((data[2] == 0x49 && data[3] == 0x1F) ||
            (data[2] == 0xC4 && data[3] == 0xFF)) {
            return true;
        }
    }
    
    return false;
}

/**
 * @brief Extract FIC from ETI frame
 *
 * @param frame ETI frame data
 * @return FIC data (96 bytes for Mode I)
 */
inline QByteArray extractFICFromFrame(const QByteArray& frame) {
    if (frame.size() < 6144) {
        return QByteArray();
    }
    
    // FIC starts at offset 8, 96 bytes for Mode I
    return frame.mid(8, 96);
}

/**
 * @brief Extract MSC from ETI frame
 *
 * @param frame ETI frame data
 * @return MSC data
 */
inline QByteArray extractMSCFromFrame(const QByteArray& frame) {
    if (frame.size() < 6144) {
        return QByteArray();
    }
    
    // MSC starts after FIC (offset 8 + 96 = 104)
    // MSC size depends on configuration, typically ~4608 bytes
    return frame.mid(104);
}

} // namespace TestUtils
