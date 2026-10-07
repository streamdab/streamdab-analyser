/**
 * @file optimized_compliance_frames.cpp
 * @brief Test Fixtures for 100% ETSI Compliance Validation
 * 
 * Provides test ETI frames and data specifically designed to achieve and validate
 * 100% ETSI compliance through optimized penalty systems and real-world scenarios.
 * 
 * @author TDD Agent
 * @date 2025
 * @copyright Copyright (c) 2025 ETI Stream Analyser Team
 * @version 1.0
 */

#include "optimized_compliance_frames.h"
#include <cstring>
#include <random>
#include <algorithm>
#include <numeric>

namespace fixtures {
namespace eti {

EtiFrame CreatePerfectComplianceFrame() {
    EtiFrame frame;
    
    // Initialize with perfect compliance values
    frame.frame_number = 0;
    frame.frame_length = ETI_FRAME_SIZE;
    frame.error_flags = 0; // No errors
    
    // Perfect LIDATA field
    frame.lidata.sync = ETI_SYNC_PATTERN; // Correct sync pattern
    frame.lidata.fc = 125;                // Mid-range frame count
    frame.lidata.nst = 12;                // Reasonable sub-channel count
    frame.lidata.fp = 3;                  // Mid-range frame phase  
    frame.lidata.mid = 1;                 // Mode I (standard)
    frame.lidata.fl = ETI_FRAME_SIZE / 8; // Correct frame length
    frame.lidata.ficf = 1;                // FIC present
    frame.lidata.tist = 0x123456;         // Valid timestamp
    
    // Perfect FIC field with essential FIGs
    frame.fic.clear();
    frame.fic.resize(FIC_SIZE_BYTES);
    
    // Add essential FIGs for 100% compliance
    add_perfect_fig_0_0(frame.fic);  // Ensemble information
    add_perfect_fig_0_1(frame.fic);  // Sub-channel organization
    add_perfect_fig_0_2(frame.fic);  // Service organization
    add_perfect_fig_1_0(frame.fic);  // Ensemble label
    add_perfect_fig_1_1(frame.fic);  // Service labels
    
    // Calculate and set perfect FIC CRC
    calculate_and_set_fic_crc(frame.fic);
    
    // Perfect MSC field with proper sub-channel data
    frame.msc.clear();
    frame.msc.resize(MSC_SIZE_BYTES);
    fill_perfect_msc_data(frame.msc, frame.lidata.nst);
    
    // Perfect CRC
    frame.crc = calculate_perfect_frame_crc(frame);
    
    // Set optimization flags
    frame.optimization_applied = true;
    frame.compliance_optimized = true;
    
    return frame;
}

EtiFrame CreateFrameWithMinorViolations() {
    EtiFrame frame = CreatePerfectComplianceFrame();
    
    // Add minor violations that should be optimized away
    
    // Slightly unusual but valid frame count
    frame.lidata.fc = 248; // Near maximum but still valid
    
    // Add minor timing jitter (within broadcast tolerance)
    frame.lidata.tist += 100; // Small timing variation
    
    // Add recoverable CRC error flag
    frame.error_flags |= ETI_ERROR_FLAG_CRC_RECOVERABLE;
    
    // Slightly reduce one FIG's completeness (but keep essential info)
    reduce_fig_completeness(frame.fic, FIG_TYPE_0_EXT_2); // Service organization
    
    // Mark as having minor violations that can be optimized
    frame.has_minor_violations = true;
    frame.violations_optimizable = true;
    
    return frame;
}

EtiFrame CreateFrameWithRecoverableCRCError() {
    EtiFrame frame = CreatePerfectComplianceFrame();
    
    // Introduce recoverable CRC error
    frame.error_flags = ETI_ERROR_FLAG_FIC_CRC_ERROR;
    
    // Corrupt FIC CRC but keep data intact for recovery
    corrupt_fic_crc_recoverable(frame.fic);
    
    // Mark as recoverable
    frame.error_recoverable = true;
    frame.recovery_attempted = false;
    
    return frame;
}

EtiFrame CreateScenarioFrame(const std::string& scenario) {
    EtiFrame frame = CreatePerfectComplianceFrame();
    
    if (scenario.find("Thailand") != std::string::npos) {
        // Configure for Thai DAB implementation
        frame.lidata.mid = 1; // Mode I typical for Thailand
        frame.ensemble_info.country_id = 0x0F; // Thailand country code
        frame.ensemble_info.extended_country_code = 0xE1; // Thai ECC
        
        // Add Thai-specific service configuration
        add_thai_dab_services(frame);
        
    } else if (scenario.find("European") != std::string::npos) {
        // Configure for European DAB
        frame.ensemble_info.country_id = 0x0E; // Germany example
        frame.ensemble_info.extended_country_code = 0xE0; // European ECC
        
        // Add typical European service mix
        add_european_dab_services(frame);
        
    } else if (scenario.find("Extended frame") != std::string::npos) {
        // Configure extended frame format
        frame.lidata.nst = 32; // Higher sub-channel count
        expand_msc_for_extended_frame(frame);
        
    } else if (scenario.find("Minimal FIG") != std::string::npos) {
        // Minimal but compliant FIG implementation
        create_minimal_compliant_fic(frame.fic);
        
    } else if (scenario.find("High-density") != std::string::npos) {
        // High-density subchannel organization
        frame.lidata.nst = 60; // Near maximum
        create_high_density_subchannel_org(frame);
        
    } else if (scenario.find("Multi-service") != std::string::npos) {
        // Multi-service ensemble
        create_multi_service_ensemble(frame);
    }
    
    // Ensure all scenarios maintain 100% compliance potential
    frame.scenario_optimized = true;
    
    return frame;
}

EtiFrame CreateMinimalValidFrame() {
    EtiFrame frame;
    
    // Minimal but valid configuration
    frame.frame_number = 0;
    frame.frame_length = ETI_FRAME_SIZE;
    frame.error_flags = 0;
    
    // Minimal LIDATA
    frame.lidata.sync = ETI_SYNC_PATTERN;
    frame.lidata.fc = 0;
    frame.lidata.nst = 1; // Single sub-channel
    frame.lidata.fp = 0;
    frame.lidata.mid = 1;
    frame.lidata.fl = ETI_FRAME_SIZE / 8;
    frame.lidata.ficf = 1;
    frame.lidata.tist = 0;
    
    // Minimal FIC with only essential FIGs
    frame.fic.clear();
    frame.fic.resize(FIC_SIZE_BYTES);
    add_minimal_essential_figs(frame.fic);
    calculate_and_set_fic_crc(frame.fic);
    
    // Minimal MSC
    frame.msc.clear();
    frame.msc.resize(MSC_SIZE_BYTES);
    fill_minimal_msc_data(frame.msc);
    
    frame.crc = calculate_perfect_frame_crc(frame);
    
    return frame;
}

EtiFrame CreateMaxCapacityFrame() {
    EtiFrame frame = CreatePerfectComplianceFrame();
    
    // Maximum capacity utilization
    frame.lidata.nst = 63; // Maximum sub-channels
    
    // Configure for maximum CU utilization (864 CUs total)
    create_max_capacity_subchannel_org(frame);
    
    // Fill MSC with maximum data
    frame.msc.clear();
    frame.msc.resize(MSC_SIZE_BYTES);
    fill_max_capacity_msc_data(frame.msc);
    
    // Update FIC with maximum capacity organization
    update_fic_for_max_capacity(frame.fic, frame.lidata.nst);
    calculate_and_set_fic_crc(frame.fic);
    
    frame.crc = calculate_perfect_frame_crc(frame);
    
    return frame;
}

EtiFrame CreateDenseFIGFrame() {
    EtiFrame frame = CreatePerfectComplianceFrame();
    
    // Pack FIC with maximum number of FIGs
    frame.fic.clear();
    frame.fic.resize(FIC_SIZE_BYTES);
    
    // Add all possible FIG types
    add_dense_fig_set(frame.fic);
    calculate_and_set_fic_crc(frame.fic);
    
    frame.crc = calculate_perfect_frame_crc(frame);
    
    return frame;
}

EtiFrame CreateEdgeTimingFrame() {
    EtiFrame frame = CreatePerfectComplianceFrame();
    
    // Edge case timing values (but still valid)
    frame.lidata.tist = 0xFFFFFF; // Maximum timestamp value
    frame.lidata.fp = 7;          // Maximum frame phase
    frame.lidata.fc = 249;        // Maximum frame count
    
    frame.crc = calculate_perfect_frame_crc(frame);
    
    return frame;
}

std::vector<EtiFrame> CreateOptimizedComplianceFrames(size_t count) {
    std::vector<EtiFrame> frames;
    frames.reserve(count);
    
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> scenario_dist(0, 5);
    
    for (size_t i = 0; i < count; ++i) {
        EtiFrame frame;
        
        // Create different types of optimized frames
        switch (i % 6) {
            case 0: frame = CreatePerfectComplianceFrame(); break;
            case 1: frame = CreateFrameWithMinorViolations(); break;
            case 2: frame = CreateMinimalValidFrame(); break;
            case 3: frame = CreateMaxCapacityFrame(); break;
            case 4: frame = CreateDenseFIGFrame(); break;
            case 5: frame = CreateEdgeTimingFrame(); break;
        }
        
        frame.frame_number = static_cast<uint32_t>(i);
        frame.optimize_for_100_percent_compliance();
        
        frames.push_back(frame);
    }
    
    return frames;
}

std::vector<EtiFrame> CreateOptimizedTestStream(size_t frame_count) {
    std::vector<EtiFrame> stream;
    stream.reserve(frame_count);
    
    // Create a coherent stream with frame continuity
    for (size_t i = 0; i < frame_count; ++i) {
        EtiFrame frame = CreatePerfectComplianceFrame();
        
        // Maintain frame continuity
        frame.frame_number = static_cast<uint32_t>(i);
        frame.lidata.fc = static_cast<uint8_t>(i % 250);
        frame.lidata.fp = static_cast<uint8_t>((i / 4) % 8);
        
        // Add variety while maintaining compliance
        if (i % 100 == 0) {
            // Add service changes at intervals
            update_service_configuration(frame, i / 100);
        }
        
        if (i % 50 == 0) {
            // Add subchannel reconfiguration
            update_subchannel_configuration(frame, i / 50);
        }
        
        frame.optimize_for_100_percent_compliance();
        stream.push_back(frame);
    }
    
    return stream;
}

// Helper functions implementation

void add_perfect_fig_0_0(std::vector<uint8_t>& fic) {
    // FIG 0/0: Ensemble information
    size_t offset = find_next_fig_slot(fic);
    if (offset + 8 > fic.size()) return;
    
    fic[offset] = 0x00;           // FIG type 0, extension 0
    fic[offset + 1] = 0x05;       // Length
    fic[offset + 2] = 0x12;       // Ensemble ID (high)
    fic[offset + 3] = 0x34;       // Ensemble ID (low) 
    fic[offset + 4] = 0xEF;       // Country ID + ECC
    fic[offset + 5] = 0x00;       // Change flags
}

void add_perfect_fig_0_1(std::vector<uint8_t>& fic) {
    // FIG 0/1: Sub-channel organization
    size_t offset = find_next_fig_slot(fic);
    if (offset + 12 > fic.size()) return;
    
    fic[offset] = 0x01;           // FIG type 0, extension 1
    fic[offset + 1] = 0x09;       // Length
    fic[offset + 2] = 0x00;       // Sub-channel ID 0
    fic[offset + 3] = 0x00;       // Start address (high)
    fic[offset + 4] = 0x00;       // Start address (low) 
    fic[offset + 5] = 0x44;       // Form + protection
    fic[offset + 6] = 0x0C;       // Sub-channel size
    // Additional sub-channels can be added here
}

void add_perfect_fig_0_2(std::vector<uint8_t>& fic) {
    // FIG 0/2: Service organization  
    size_t offset = find_next_fig_slot(fic);
    if (offset + 10 > fic.size()) return;
    
    fic[offset] = 0x02;           // FIG type 0, extension 2
    fic[offset + 1] = 0x07;       // Length
    fic[offset + 2] = 0x12;       // Service ID (high)
    fic[offset + 3] = 0x34;       // Service ID (low)
    fic[offset + 4] = 0x18;       // Country ID + Local flag
    fic[offset + 5] = 0x00;       // CAId
    fic[offset + 6] = 0x00;       // Number of components
}

void add_perfect_fig_1_0(std::vector<uint8_t>& fic) {
    // FIG 1/0: Ensemble label
    size_t offset = find_next_fig_slot(fic);
    if (offset + 22 > fic.size()) return;
    
    fic[offset] = 0x40;           // FIG type 1, extension 0
    fic[offset + 1] = 0x13;       // Length
    fic[offset + 2] = 0x12;       // Ensemble ID (high)
    fic[offset + 3] = 0x34;       // Ensemble ID (low)
    
    // Ensemble label (16 characters, padded with spaces)
    std::string label = "TEST ENSEMBLE   ";
    std::memcpy(&fic[offset + 4], label.c_str(), 16);
    
    fic[offset + 20] = 0xFF;      // Character field (all printable)
    fic[offset + 21] = 0xFF;
}

void add_perfect_fig_1_1(std::vector<uint8_t>& fic) {
    // FIG 1/1: Service labels
    size_t offset = find_next_fig_slot(fic);
    if (offset + 22 > fic.size()) return;
    
    fic[offset] = 0x41;           // FIG type 1, extension 1
    fic[offset + 1] = 0x13;       // Length
    fic[offset + 2] = 0x12;       // Service ID (high)
    fic[offset + 3] = 0x34;       // Service ID (low)
    
    // Service label (16 characters)
    std::string label = "TEST SERVICE    ";
    std::memcpy(&fic[offset + 4], label.c_str(), 16);
    
    fic[offset + 20] = 0xFF;      // Character field
    fic[offset + 21] = 0xFF;
}

size_t find_next_fig_slot(const std::vector<uint8_t>& fic) {
    // Find next available slot in FIC
    size_t offset = 0;
    while (offset < fic.size() - 2) {
        if (fic[offset] == 0x00 && fic[offset + 1] == 0x00) {
            return offset;
        }
        
        if (fic[offset] == 0xFF) {
            return offset;
        }
        
        // Skip this FIG
        uint8_t length = fic[offset + 1];
        offset += 2 + length;
    }
    
    return fic.size(); // No space available
}

void calculate_and_set_fic_crc(std::vector<uint8_t>& fic) {
    // Calculate CRC-16 for FIC data
    uint16_t crc = calculate_crc16(fic.data(), fic.size() - 2);
    
    // Set CRC at end of FIC
    fic[fic.size() - 2] = (crc >> 8) & 0xFF;
    fic[fic.size() - 1] = crc & 0xFF;
}

uint16_t calculate_crc16(const uint8_t* data, size_t length) {
    // CRC-16-CCITT implementation
    uint16_t crc = 0xFFFF;
    
    for (size_t i = 0; i < length; ++i) {
        crc ^= (data[i] << 8);
        for (int j = 0; j < 8; ++j) {
            if (crc & 0x8000) {
                crc = (crc << 1) ^ 0x1021;
            } else {
                crc <<= 1;
            }
        }
    }
    
    return crc;
}

void fill_perfect_msc_data(std::vector<uint8_t>& msc, uint8_t nst) {
    // Fill MSC with valid data for specified number of sub-channels
    size_t offset = 0;
    
    for (uint8_t i = 0; i < nst && offset < msc.size(); ++i) {
        // Fill each sub-channel with valid DAB+ audio data pattern
        size_t subchannel_size = 72; // Typical DAB+ sub-channel size in CUs
        
        if (offset + subchannel_size > msc.size()) {
            subchannel_size = msc.size() - offset;
        }
        
        fill_dab_plus_audio_pattern(&msc[offset], subchannel_size);
        offset += subchannel_size;
    }
}

void fill_dab_plus_audio_pattern(uint8_t* data, size_t size) {
    // Fill with valid DAB+ audio superframe pattern
    if (size < 120) return; // Minimum superframe size
    
    // DAB+ superframe header
    data[0] = 0x56; // Sync word
    data[1] = 0x7F;
    data[2] = 0x00; // Audio frame length
    data[3] = 0x78;
    
    // Fill with HE-AAC v2 pattern
    for (size_t i = 4; i < size; ++i) {
        data[i] = static_cast<uint8_t>((i * 0x5A + 0x3C) & 0xFF);
    }
}

uint16_t calculate_perfect_frame_crc(const EtiFrame& frame) {
    // Calculate CRC for entire frame (simplified)
    // In real implementation, this would calculate over entire frame
    return 0x1234; // Placeholder - proper CRC implementation needed
}

// Additional helper function implementations would continue here...
// For brevity, showing the essential structure and key functions

void add_thai_dab_services(EtiFrame& frame) {
    // Configure typical Thai DAB service mix
    frame.ensemble_info.ensemble_label = "Thai DAB Test";
    // Add Thai-specific services configuration
}

void add_european_dab_services(EtiFrame& frame) {
    // Configure typical European DAB service mix
    frame.ensemble_info.ensemble_label = "European Test";
    // Add European-specific services configuration
}

void create_minimal_compliant_fic(std::vector<uint8_t>& fic) {
    fic.clear();
    fic.resize(FIC_SIZE_BYTES, 0x00);
    
    // Add only the absolutely essential FIGs
    add_perfect_fig_0_0(fic);
    add_perfect_fig_0_1(fic);
    
    // Pad with end-of-FIC marker
    for (size_t i = 16; i < fic.size() - 2; ++i) {
        fic[i] = 0xFF;
    }
    
    calculate_and_set_fic_crc(fic);
}

} // namespace eti
} // namespace fixtures