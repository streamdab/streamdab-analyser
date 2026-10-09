#include "etsi_compliant_frames.h"
#include <cstring>
#include <algorithm>
#include <random>
#include <chrono>
#include <cmath>

namespace EtsiTestData {

// CRC-16-CCITT lookup table for ETSI compliance
static const uint16_t crc16_ccitt_table[256] = {
    0x0000, 0x1021, 0x2042, 0x3063, 0x4084, 0x50A5, 0x60C6, 0x70E7,
    0x8108, 0x9129, 0xA14A, 0xB16B, 0xC18C, 0xD1AD, 0xE1CE, 0xF1EF,
    0x1231, 0x0210, 0x3273, 0x2252, 0x52B5, 0x4294, 0x72F7, 0x62D6,
    0x9339, 0x8318, 0xB37B, 0xA35A, 0xD3BD, 0xC39C, 0xF3FF, 0xE3DE,
    0x2462, 0x3443, 0x0420, 0x1401, 0x64E6, 0x74C7, 0x44A4, 0x5485,
    0xA56A, 0xB54B, 0x8528, 0x9509, 0xE5EE, 0xF5CF, 0xC5AC, 0xD58D,
    0x3653, 0x2672, 0x1611, 0x0630, 0x76D7, 0x66F6, 0x5695, 0x46B4,
    0xB75B, 0xA77A, 0x9719, 0x8738, 0xF7DF, 0xE7FE, 0xD79D, 0xC7BC,
    0x48C4, 0x58E5, 0x6886, 0x78A7, 0x0840, 0x1861, 0x2802, 0x3823,
    0xC9CC, 0xD9ED, 0xE98E, 0xF9AF, 0x8948, 0x9969, 0xA90A, 0xB92B,
    0x5AF5, 0x4AD4, 0x7AB7, 0x6A96, 0x1A71, 0x0A50, 0x3A33, 0x2A12,
    0xDBFD, 0xCBDC, 0xFBBF, 0xEB9E, 0x9B79, 0x8B58, 0xBB3B, 0xAB1A,
    0x6CA6, 0x7C87, 0x4CE4, 0x5CC5, 0x2C22, 0x3C03, 0x0C60, 0x1C41,
    0xEDAE, 0xFD8F, 0xCDEC, 0xDDCD, 0xAD2A, 0xBD0B, 0x8D68, 0x9D49,
    0x7E97, 0x6EB6, 0x5ED5, 0x4EF4, 0x3E13, 0x2E32, 0x1E51, 0x0E70,
    0xFF9F, 0xEFBE, 0xDFDD, 0xCFFC, 0xBF1B, 0xAF3A, 0x9F59, 0x8F78,
    0x9188, 0x81A9, 0xB1CA, 0xA1EB, 0xD10C, 0xC12D, 0xF14E, 0xE16F,
    0x1080, 0x00A1, 0x30C2, 0x20E3, 0x5004, 0x4025, 0x7046, 0x6067,
    0x83B9, 0x9398, 0xA3FB, 0xB3DA, 0xC33D, 0xD31C, 0xE37F, 0xF35E,
    0x02B1, 0x1290, 0x22F3, 0x32D2, 0x4235, 0x5214, 0x6277, 0x7256,
    0xB5EA, 0xA5CB, 0x95A8, 0x8589, 0xF56E, 0xE54F, 0xD52C, 0xC50D,
    0x34E2, 0x24C3, 0x14A0, 0x0481, 0x7466, 0x6447, 0x5424, 0x4405,
    0xA7DB, 0xB7FA, 0x8799, 0x97B8, 0xE75F, 0xF77E, 0xC71D, 0xD73C,
    0x26D3, 0x36F2, 0x0691, 0x16B0, 0x6657, 0x7676, 0x4615, 0x5634,
    0xD94C, 0xC96D, 0xF90E, 0xE92F, 0x99C8, 0x89E9, 0xB98A, 0xA9AB,
    0x5844, 0x4865, 0x7806, 0x6827, 0x18C0, 0x08E1, 0x3882, 0x28A3,
    0xCB7D, 0xDB5C, 0xEB3F, 0xFB1E, 0x8BF9, 0x9BD8, 0xABBB, 0xBB9A,
    0x4A75, 0x5A54, 0x6A37, 0x7A16, 0x0AF1, 0x1AD0, 0x2AB3, 0x3A92,
    0xFD2E, 0xED0F, 0xDD6C, 0xCD4D, 0xBDAA, 0xAD8B, 0x9DE8, 0x8DC9,
    0x7C26, 0x6C07, 0x5C64, 0x4C45, 0x3CA2, 0x2C83, 0x1CE0, 0x0CC1,
    0xEF1F, 0xFF3E, 0xCF5D, 0xDF7C, 0xAF9B, 0xBFBA, 0x8FD9, 0x9FF8,
    0x6E17, 0x7E36, 0x4E55, 0x5E74, 0x2E93, 0x3EB2, 0x0ED1, 0x1EF0
};

// EtsiEtiFrame implementation
EtsiEtiFrame::EtsiEtiFrame() {
    data.fill(0);
    // Set ETSI sync pattern
    std::memcpy(data.data(), SYNC_PATTERN, SYNC_SIZE);
}

uint16_t EtsiEtiFrame::calculate_crc16(const uint8_t* data_ptr, size_t length) const {
    uint16_t crc = 0xFFFF;
    for (size_t i = 0; i < length; ++i) {
        crc = (crc << 8) ^ crc16_ccitt_table[((crc >> 8) ^ data_ptr[i]) & 0xFF];
    }
    return crc;
}

bool EtsiEtiFrame::validate_sync_pattern() const {
    return std::memcmp(data.data(), SYNC_PATTERN, SYNC_SIZE) == 0;
}

bool EtsiEtiFrame::validate_frame_structure() const {
    if (!validate_sync_pattern()) return false;
    
    // Check frame count (FC) range: 0-249
    uint8_t fc = get_frame_count();
    if (fc > 249) return false;
    
    // Check sub-channel count (NST) range: 0-63
    uint8_t nst = get_sub_channel_count();
    if (nst > 63) return false;
    
    // Check FIC flag (FICF) - should be 1 for ETI(NI)
    uint8_t ficf = (data[5] >> 7) & 0x01;
    if (ficf != 1) return false;
    
    return true;
}

bool EtsiEtiFrame::validate_crc() const {
    uint16_t stored_crc = (data[FRAME_SIZE-4] << 8) | data[FRAME_SIZE-3];
    uint16_t calculated_crc = calculate_crc16(data.data(), FRAME_SIZE - 4);
    return stored_crc == calculated_crc;
}

bool EtsiEtiFrame::is_etsi_compliant() const {
    return validate_sync_pattern() && validate_frame_structure() && validate_crc();
}

uint8_t EtsiEtiFrame::get_frame_count() const {
    return data[4];
}

uint8_t EtsiEtiFrame::get_sub_channel_count() const {
    return data[5] & 0x3F; // NST is bits 0-5 of byte 5
}

uint16_t EtsiEtiFrame::get_ensemble_id() const {
    // Ensemble ID is in FIC data, typically in first FIG 0/0
    const uint8_t* fic_data = &data[SYNC_SIZE + LIDATA_SIZE];
    if (fic_data[0] == 0x00 && fic_data[1] >= 4) { // FIG 0/0 with sufficient length
        return (fic_data[2] << 8) | fic_data[3];
    }
    return 0;
}

std::vector<uint8_t> EtsiEtiFrame::get_fic_data() const {
    const uint8_t* fic_start = &data[SYNC_SIZE + LIDATA_SIZE];
    return std::vector<uint8_t>(fic_start, fic_start + FIC_SIZE);
}

std::vector<uint8_t> EtsiEtiFrame::get_msc_data() const {
    const uint8_t* msc_start = &data[SYNC_SIZE + LIDATA_SIZE + FIC_SIZE];
    return std::vector<uint8_t>(msc_start, msc_start + MSC_SIZE);
}

void EtsiEtiFrame::set_frame_count(uint8_t fc) {
    data[4] = fc % 250; // Ensure valid range
}

void EtsiEtiFrame::set_sub_channel_count(uint8_t nst) {
    data[5] = (data[5] & 0xC0) | (nst & 0x3F); // Preserve FICF and other bits
}

void EtsiEtiFrame::set_ensemble_id(uint16_t eid) {
    // Set ensemble ID in FIC FIG 0/0
    uint8_t* fic_data = &data[SYNC_SIZE + LIDATA_SIZE];
    if (fic_data[0] == 0x00) { // FIG 0/0
        fic_data[2] = (eid >> 8) & 0xFF;
        fic_data[3] = eid & 0xFF;
    }
}

void EtsiEtiFrame::update_crc() {
    uint16_t crc = calculate_crc16(data.data(), FRAME_SIZE - 4);
    data[FRAME_SIZE-4] = (crc >> 8) & 0xFF;
    data[FRAME_SIZE-3] = crc & 0xFF;
    data[FRAME_SIZE-2] = 0x00; // Reed-Solomon padding
    data[FRAME_SIZE-1] = 0x00; // Reed-Solomon padding
}

// German public radio ensemble
static EtsiEtiFrame create_german_ensemble() {
    EtsiEtiFrame frame;
    
    // LIDATA field setup
    frame.data[4] = 0;      // Frame Count = 0
    frame.data[5] = 0x84;   // FICF=1, NST=4 (4 sub-channels)
    frame.data[6] = 0x12;   // FP=1, MID=1 (Mode I), FL high
    frame.data[7] = 0x00;   // FL low
    
    // TIST (Time Stamp) - bytes 8-11
    uint32_t tist = 0x00000000; // Initial timestamp
    frame.data[8] = (tist >> 24) & 0xFF;
    frame.data[9] = (tist >> 16) & 0xFF;
    frame.data[10] = (tist >> 8) & 0xFF;
    frame.data[11] = tist & 0xFF;
    
    // FIC data with German ensemble configuration
    uint8_t* fic = &frame.data[12];
    
    // FIG 0/0: Ensemble information
    fic[0] = 0x00;  // FIG type 0/0
    fic[1] = 0x05;  // Length = 5
    fic[2] = 0xD0;  // Ensemble ID (high) - German country code
    fic[3] = 0x01;  // Ensemble ID (low)
    fic[4] = 0x00;  // Change flags
    fic[5] = 0x00;  // CIF count (high)
    fic[6] = 0x01;  // CIF count (low)
    
    // FIG 0/1: Sub-channel organization (4 sub-channels)
    fic[7] = 0x01;   // FIG type 0/1
    fic[8] = 0x10;   // Length = 16 (4 sub-channels × 4 bytes)
    
    // Sub-channel 0: Deutschlandfunk (64 kbps)
    fic[9] = 0x00;   // Sub-channel ID = 0
    fic[10] = 0x00;  // Start address = 0
    fic[11] = 0x00;  // Start address (high)
    fic[12] = 0x40;  // Size = 64 CUs, Protection level 2
    
    // Sub-channel 1: WDR Radio (64 kbps)  
    fic[13] = 0x01;  // Sub-channel ID = 1
    fic[14] = 0x40;  // Start address = 64
    fic[15] = 0x00;  // Start address (high)
    fic[16] = 0x40;  // Size = 64 CUs, Protection level 2
    
    // Sub-channel 2: Bayern 1 (96 kbps)
    fic[17] = 0x02;  // Sub-channel ID = 2
    fic[18] = 0x80;  // Start address = 128
    fic[19] = 0x00;  // Start address (high)
    fic[20] = 0x60;  // Size = 96 CUs, Protection level 2
    
    // Sub-channel 3: HR Info (48 kbps)
    fic[21] = 0x03;  // Sub-channel ID = 3
    fic[22] = 0xE0;  // Start address = 224
    fic[23] = 0x00;  // Start address (high)
    fic[24] = 0x30;  // Size = 48 CUs, Protection level 1
    
    // Fill remaining FIC with padding
    std::fill(fic + 25, fic + 32, 0xFF);
    
    // MSC: Generate realistic audio data patterns
    uint8_t* msc = &frame.data[44];
    std::mt19937 rng(12345); // Fixed seed for reproducible data
    
    // Fill each sub-channel with distinctive patterns
    size_t offset = 0;
    
    // Sub-channel 0: Deutschlandfunk (512 bytes = 64 CUs × 8 bytes/CU)
    for (size_t i = 0; i < 512; ++i) {
        msc[offset++] = static_cast<uint8_t>((0xDF + i) & 0xFF); // DF pattern
    }
    
    // Sub-channel 1: WDR Radio (512 bytes)
    for (size_t i = 0; i < 512; ++i) {
        msc[offset++] = static_cast<uint8_t>((0xWD + i) & 0xFF); // WD pattern (simplified)
    }
    
    // Fill remaining MSC
    while (offset < 6096) {
        msc[offset++] = static_cast<uint8_t>(rng() & 0xFF);
    }
    
    frame.update_crc();
    return frame;
}

const EtsiEtiFrame GERMAN_PUBLIC_RADIO_ENSEMBLE = create_german_ensemble();

// UK BBC Radio ensemble
static EtsiEtiFrame create_uk_ensemble() {
    EtsiEtiFrame frame;
    
    // LIDATA field for UK ensemble
    frame.data[4] = 0;      // Frame Count = 0
    frame.data[5] = 0x85;   // FICF=1, NST=5 (BBC Radio 1,2,3,4,5)
    frame.data[6] = 0x12;   // FP=1, MID=1
    frame.data[7] = 0x00;
    
    // TIST
    uint32_t tist = 0x12345678;
    frame.data[8] = (tist >> 24) & 0xFF;
    frame.data[9] = (tist >> 16) & 0xFF;
    frame.data[10] = (tist >> 8) & 0xFF;
    frame.data[11] = tist & 0xFF;
    
    // FIC data with UK ensemble configuration
    uint8_t* fic = &frame.data[12];
    
    // FIG 0/0: UK Ensemble
    fic[0] = 0x00;  // FIG type 0/0
    fic[1] = 0x05;  // Length
    fic[2] = 0xE1;  // UK country code + ensemble ID (high)
    fic[3] = 0x01;  // Ensemble ID (low)
    fic[4] = 0x00;  // Change flags
    fic[5] = 0x00;  // CIF count (high)
    fic[6] = 0x02;  // CIF count (low)
    
    // FIG 0/1: BBC Radio services organization
    fic[7] = 0x01;   // FIG type 0/1
    fic[8] = 0x14;   // Length = 20 (5 sub-channels)
    
    // BBC Radio 1 (128 kbps)
    fic[9] = 0x00;   // Sub-channel ID = 0
    fic[10] = 0x00;  // Start = 0
    fic[11] = 0x00;
    fic[12] = 0x80;  // Size = 128 CUs
    
    // BBC Radio 2 (128 kbps)
    fic[13] = 0x01; fic[14] = 0x80; fic[15] = 0x00; fic[16] = 0x80;
    
    // BBC Radio 3 (160 kbps)
    fic[17] = 0x02; fic[18] = 0x00; fic[19] = 0x01; fic[20] = 0xA0;
    
    // BBC Radio 4 (96 kbps)
    fic[21] = 0x03; fic[22] = 0xA0; fic[23] = 0x01; fic[24] = 0x60;
    
    // BBC Radio 5 Live (96 kbps)
    fic[25] = 0x04; fic[26] = 0x00; fic[27] = 0x02; fic[28] = 0x60;
    
    // Padding
    std::fill(fic + 29, fic + 32, 0xFF);
    
    // MSC with BBC-specific patterns
    uint8_t* msc = &frame.data[44];
    std::mt19937 rng(0xBBC);
    
    for (size_t i = 0; i < 6096; ++i) {
        msc[i] = static_cast<uint8_t>((0xBC + i) & 0xFF);
    }
    
    frame.update_crc();
    return frame;
}

const EtsiEtiFrame UK_BBC_RADIO_ENSEMBLE = create_uk_ensemble();

// French Radio France ensemble  
static EtsiEtiFrame create_french_ensemble() {
    EtsiEtiFrame frame;
    
    frame.data[4] = 0;      // Frame Count = 0
    frame.data[5] = 0x86;   // FICF=1, NST=6 (French radio services)
    frame.data[6] = 0x12;   // FP=1, MID=1
    frame.data[7] = 0x00;
    
    // TIST
    uint32_t tist = 0xFEEDFACE;
    frame.data[8] = (tist >> 24) & 0xFF;
    frame.data[9] = (tist >> 16) & 0xFF;
    frame.data[10] = (tist >> 8) & 0xFF;
    frame.data[11] = tist & 0xFF;
    
    // FIC with French configuration
    uint8_t* fic = &frame.data[12];
    
    // FIG 0/0: French ensemble
    fic[0] = 0x00; fic[1] = 0x05;
    fic[2] = 0xF0; fic[3] = 0x01; // French country code
    fic[4] = 0x00; fic[5] = 0x00; fic[6] = 0x03;
    
    // FIG 0/1: French radio organization
    fic[7] = 0x01; fic[8] = 0x18; // 6 services
    
    // France Inter, France Info, France Culture, etc.
    const uint8_t french_subchannels[6][4] = {
        {0x00, 0x00, 0x00, 0x60}, // France Inter (96 kbps)
        {0x01, 0x60, 0x00, 0x40}, // France Info (64 kbps)
        {0x02, 0xA0, 0x00, 0x60}, // France Culture (96 kbps)
        {0x03, 0x00, 0x01, 0x40}, // France Musique (64 kbps)
        {0x04, 0x40, 0x01, 0x40}, // FIP (64 kbps)
        {0x05, 0x80, 0x01, 0x30}  // France Bleu (48 kbps)
    };
    
    for (int i = 0; i < 6; ++i) {
        fic[9 + i*4] = french_subchannels[i][0];
        fic[10 + i*4] = french_subchannels[i][1];
        fic[11 + i*4] = french_subchannels[i][2];
        fic[12 + i*4] = french_subchannels[i][3];
    }
    
    // MSC with French patterns
    uint8_t* msc = &frame.data[44];
    for (size_t i = 0; i < 6096; ++i) {
        msc[i] = static_cast<uint8_t>((0xFE + i) & 0xFF);
    }
    
    frame.update_crc();
    return frame;
}

const EtsiEtiFrame FRENCH_RADIO_FRANCE_ENSEMBLE = create_french_ensemble();

// Multi-format ensemble (DAB + DAB+ + data)
static EtsiEtiFrame create_multi_format_ensemble() {
    EtsiEtiFrame frame;
    
    frame.data[4] = 0;
    frame.data[5] = 0x88;   // NST=8 (mixed services)
    frame.data[6] = 0x12;
    frame.data[7] = 0x00;
    
    // TIST
    uint32_t tist = 0xDEADBEEF;
    frame.data[8] = (tist >> 24) & 0xFF;
    frame.data[9] = (tist >> 16) & 0xFF;
    frame.data[10] = (tist >> 8) & 0xFF;
    frame.data[11] = tist & 0xFF;
    
    // FIC for multi-format
    uint8_t* fic = &frame.data[12];
    
    fic[0] = 0x00; fic[1] = 0x05;
    fic[2] = 0xAB; fic[3] = 0xCD; // Mixed ensemble ID
    fic[4] = 0x00; fic[5] = 0x00; fic[6] = 0x04;
    
    // Mixed service organization
    fic[7] = 0x01; fic[8] = 0x20; // 8 services × 4 bytes
    
    // DAB services (0-3), DAB+ services (4-6), data service (7)
    for (int i = 0; i < 8; ++i) {
        fic[9 + i*4] = i;                        // Sub-channel ID
        fic[10 + i*4] = i * 32;                  // Start address
        fic[11 + i*4] = 0x00;                    // Start high
        fic[12 + i*4] = (i < 7) ? 0x40 : 0x20;   // Size (data service smaller)
    }
    
    // MSC with mixed content patterns
    uint8_t* msc = &frame.data[44];
    std::mt19937 rng(0xMIXED);
    
    for (size_t i = 0; i < 6096; ++i) {
        // Create pattern indicating content type
        if (i < 1024) {
            msc[i] = 0xDA; // DAB pattern
        } else if (i < 2048) {
            msc[i] = 0xD1; // DAB+ pattern
        } else if (i < 2304) {
            msc[i] = 0xDD; // Data pattern
        } else {
            msc[i] = static_cast<uint8_t>(rng() & 0xFF);
        }
    }
    
    frame.update_crc();
    return frame;
}

const EtsiEtiFrame MULTI_FORMAT_ENSEMBLE = create_multi_format_ensemble();

// Minimal valid ensemble
static EtsiEtiFrame create_minimal_ensemble() {
    EtsiEtiFrame frame;
    
    frame.data[4] = 0;
    frame.data[5] = 0x81;   // NST=1 (single service)
    frame.data[6] = 0x10;   // Minimal configuration
    frame.data[7] = 0x00;
    
    // Minimal TIST
    std::fill(&frame.data[8], &frame.data[12], 0x00);
    
    // Minimal FIC
    uint8_t* fic = &frame.data[12];
    fic[0] = 0x00; fic[1] = 0x05;
    fic[2] = 0x00; fic[3] = 0x01; // Minimal ensemble ID
    fic[4] = 0x00; fic[5] = 0x00; fic[6] = 0x00;
    
    fic[7] = 0x01; fic[8] = 0x04; // Single sub-channel
    fic[9] = 0x00; fic[10] = 0x00; fic[11] = 0x00; fic[12] = 0x40;
    
    std::fill(fic + 13, fic + 32, 0xFF);
    
    // Minimal MSC
    uint8_t* msc = &frame.data[44];
    std::fill(msc, msc + 6096, 0x00);
    
    frame.update_crc();
    return frame;
}

const EtsiEtiFrame MINIMAL_VALID_ENSEMBLE = create_minimal_ensemble();

// Maximum capacity ensemble
static EtsiEtiFrame create_maximum_ensemble() {
    EtsiEtiFrame frame;
    
    frame.data[4] = 0;
    frame.data[5] = 0xBF;   // NST=63 (maximum sub-channels)
    frame.data[6] = 0x12;
    frame.data[7] = 0x00;
    
    uint32_t tist = 0xFFFFFFFF;
    frame.data[8] = (tist >> 24) & 0xFF;
    frame.data[9] = (tist >> 16) & 0xFF;
    frame.data[10] = (tist >> 8) & 0xFF;
    frame.data[11] = tist & 0xFF;
    
    // FIC for maximum configuration (truncated due to space)
    uint8_t* fic = &frame.data[12];
    fic[0] = 0x00; fic[1] = 0x05;
    fic[2] = 0xFF; fic[3] = 0xFF; // Maximum ensemble ID
    fic[4] = 0x00; fic[5] = 0x00; fic[6] = 0xFF;
    
    fic[7] = 0x01; fic[8] = 0x18; // Partial sub-channel data (space limited)
    
    // Configure first 6 sub-channels as example
    for (int i = 0; i < 6; ++i) {
        fic[9 + i*4] = i;
        fic[10 + i*4] = i * 10;
        fic[11 + i*4] = 0x00;
        fic[12 + i*4] = 0x0A; // Small size to fit many
    }
    
    std::fill(fic + 33, fic + 32, 0xFF);
    
    // MSC packed with maximum data
    uint8_t* msc = &frame.data[44];
    for (size_t i = 0; i < 6096; ++i) {
        msc[i] = static_cast<uint8_t>((i + 0xFF) & 0xFF);
    }
    
    frame.update_crc();
    return frame;
}

const EtsiEtiFrame MAXIMUM_CAPACITY_ENSEMBLE = create_maximum_ensemble();

// Error frames namespace
namespace ErrorFrames {
    static EtsiEtiFrame create_invalid_sync() {
        EtsiEtiFrame frame = GERMAN_PUBLIC_RADIO_ENSEMBLE;
        frame.data[0] = 0xFF; // Corrupt sync
        frame.data[1] = 0xFF;
        frame.data[2] = 0xFF;
        frame.data[3] = 0xFF;
        return frame;
    }
    
    const EtsiEtiFrame INVALID_SYNC_FRAME = create_invalid_sync();
    
    static EtsiEtiFrame create_corrupted_fic() {
        EtsiEtiFrame frame = GERMAN_PUBLIC_RADIO_ENSEMBLE;
        std::fill(&frame.data[12], &frame.data[44], 0xAA); // Corrupt entire FIC
        frame.update_crc();
        return frame;
    }
    
    const EtsiEtiFrame CORRUPTED_FIC_FRAME = create_corrupted_fic();
    
    static EtsiEtiFrame create_invalid_crc() {
        EtsiEtiFrame frame = GERMAN_PUBLIC_RADIO_ENSEMBLE;
        frame.data[FRAME_SIZE-4] = 0xFF; // Wrong CRC
        frame.data[FRAME_SIZE-3] = 0xFF;
        return frame;
    }
    
    const EtsiEtiFrame INVALID_CRC_FRAME = create_invalid_crc();
    
    static EtsiEtiFrame create_timing_error() {
        EtsiEtiFrame frame = GERMAN_PUBLIC_RADIO_ENSEMBLE;
        frame.data[4] = 250; // Invalid frame count (>249)
        frame.update_crc();
        return frame;
    }
    
    const EtsiEtiFrame TIMING_ERROR_FRAME = create_timing_error();
    
    static EtsiEtiFrame create_malformed_msc() {
        EtsiEtiFrame frame = GERMAN_PUBLIC_RADIO_ENSEMBLE;
        // Create impossible sub-channel layout in MSC
        uint8_t* msc = &frame.data[44];
        std::fill(msc, msc + 100, 0xFF); // Invalid start pattern
        frame.update_crc();
        return frame;
    }
    
    const EtsiEtiFrame MALFORMED_MSC_FRAME = create_malformed_msc();
}

// Stream simulation implementation
std::vector<EtsiEtiFrame> EtsiStreamSimulator::generate_stream(
    const StreamConfig& config, 
    double duration_seconds) {
    
    size_t frame_count = static_cast<size_t>(duration_seconds * config.frame_rate);
    std::vector<EtsiEtiFrame> stream;
    stream.reserve(frame_count);
    
    std::mt19937 rng(static_cast<uint32_t>(std::chrono::steady_clock::now().time_since_epoch().count()));
    std::uniform_real_distribution<double> error_dist(0.0, 1.0);
    
    for (size_t i = 0; i < frame_count; ++i) {
        EtsiEtiFrame frame;
        
        // Set frame count (wraps at 250)
        frame.set_frame_count(static_cast<uint8_t>(i % 250));
        
        // Set ensemble info
        frame.set_ensemble_id(config.ensemble_id);
        frame.set_sub_channel_count(config.service_count);
        
        // Update TIST with frame timing
        uint32_t tist = static_cast<uint32_t>(i * 24000); // 24ms per frame in µs
        frame.data[8] = (tist >> 24) & 0xFF;
        frame.data[9] = (tist >> 16) & 0xFF;
        frame.data[10] = (tist >> 8) & 0xFF;
        frame.data[11] = tist & 0xFF;
        
        // Generate MSC content
        uint8_t* msc = &frame.data[44];
        for (size_t j = 0; j < 6096; ++j) {
            msc[j] = static_cast<uint8_t>((i + j) & 0xFF);
        }
        
        // Inject errors if configured
        if (config.include_errors && error_dist(rng) < config.error_rate) {
            // Inject random error
            int error_type = rng() % 5;
            switch (error_type) {
                case 0: frame.data[0] = 0xFF; break; // Sync error
                case 1: frame.data[5] = 0xFF; break; // Header error
                case 2: frame.data[15] = 0xFF; break; // FIC error
                case 3: msc[100] = 0xFF; break; // MSC error
                case 4: /* Keep good CRC for CRC error test */ break;
            }
        }
        
        frame.update_crc();
        stream.push_back(frame);
    }
    
    return stream;
}

std::vector<EtsiEtiFrame> EtsiStreamSimulator::generate_performance_stream(size_t frame_count) {
    std::vector<EtsiEtiFrame> stream;
    stream.reserve(frame_count);
    
    // Pre-generate base frame for efficiency
    EtsiEtiFrame base_frame = GERMAN_PUBLIC_RADIO_ENSEMBLE;
    
    for (size_t i = 0; i < frame_count; ++i) {
        EtsiEtiFrame frame = base_frame;
        frame.set_frame_count(static_cast<uint8_t>(i % 250));
        
        // Minimal MSC variation for performance
        uint8_t* msc = &frame.data[44];
        msc[0] = static_cast<uint8_t>(i & 0xFF);
        msc[1] = static_cast<uint8_t>((i >> 8) & 0xFF);
        
        frame.update_crc();
        stream.push_back(frame);
    }
    
    return stream;
}

std::vector<EtsiEtiFrame> EtsiStreamSimulator::generate_timing_test_stream(size_t frame_count) {
    std::vector<EtsiEtiFrame> stream;
    stream.reserve(frame_count);
    
    auto start_time = std::chrono::steady_clock::now();
    
    for (size_t i = 0; i < frame_count; ++i) {
        EtsiEtiFrame frame = GERMAN_PUBLIC_RADIO_ENSEMBLE;
        frame.set_frame_count(static_cast<uint8_t>(i % 250));
        
        // Calculate precise TIST based on real timing
        auto frame_time = start_time + std::chrono::microseconds(i * 24000);
        auto tist_us = std::chrono::duration_cast<std::chrono::microseconds>(
            frame_time.time_since_epoch()).count();
        
        uint32_t tist = static_cast<uint32_t>(tist_us & 0xFFFFFFFF);
        frame.data[8] = (tist >> 24) & 0xFF;
        frame.data[9] = (tist >> 16) & 0xFF;
        frame.data[10] = (tist >> 8) & 0xFF;
        frame.data[11] = tist & 0xFF;
        
        frame.update_crc();
        stream.push_back(frame);
    }
    
    return stream;
}

// Performance test data implementation
std::vector<EtsiEtiFrame> PerformanceTestData::generate_high_speed_test_data(size_t frame_count) {
    return EtsiStreamSimulator::generate_performance_stream(frame_count);
}

std::vector<EtsiEtiFrame> PerformanceTestData::generate_stress_test_data(size_t frame_count) {
    std::vector<EtsiEtiFrame> stream;
    stream.reserve(frame_count);
    
    // Use maximum complexity frame as base
    EtsiEtiFrame base_frame = MAXIMUM_CAPACITY_ENSEMBLE;
    
    std::mt19937 rng(0xSTRESS);
    
    for (size_t i = 0; i < frame_count; ++i) {
        EtsiEtiFrame frame = base_frame;
        frame.set_frame_count(static_cast<uint8_t>(i % 250));
        
        // Vary content significantly for stress testing
        uint8_t* msc = &frame.data[44];
        for (size_t j = 0; j < 6096; j += 64) {
            uint32_t random_val = rng();
            msc[j] = (random_val >> 24) & 0xFF;
            msc[j+1] = (random_val >> 16) & 0xFF;
            msc[j+2] = (random_val >> 8) & 0xFF;
            msc[j+3] = random_val & 0xFF;
        }
        
        frame.update_crc();
        stream.push_back(frame);
    }
    
    return stream;
}

std::vector<EtsiEtiFrame> PerformanceTestData::generate_memory_test_data(size_t size_mb) {
    size_t frame_count = (size_mb * 1024 * 1024) / EtsiEtiFrame::FRAME_SIZE;
    return generate_high_speed_test_data(frame_count);
}

std::vector<EtsiEtiFrame> PerformanceTestData::generate_timing_validation_data(double duration_seconds) {
    size_t frame_count = static_cast<size_t>(duration_seconds * 250); // 250 fps
    return EtsiStreamSimulator::generate_timing_test_stream(frame_count);
}

} // namespace EtsiTestData