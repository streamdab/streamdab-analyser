#include "test_data_generators.h"
#include "etsi_compliant_frames.h"
#include <algorithm>
#include <fstream>
#include <thread>
#include <chrono>
#include <cstring>
#include <iomanip>
#include <sstream>

using namespace EtsiTestData;

namespace TestDataGenerators {

// EtiFrameGenerator implementation
EtiFrameGenerator::EtiFrameGenerator(const GeneratorConfig& config)
    : config_(config), rng_(std::random_device{}()) {
}

EtsiEtiFrame EtiFrameGenerator::generate_frame(uint32_t frame_number) {
    EtsiEtiFrame frame;
    
    // Set basic frame structure
    frame.set_frame_count(static_cast<uint8_t>(frame_number % 250));
    frame.set_sub_channel_count(config_.service_count);
    frame.set_ensemble_id(config_.ensemble_id);
    
    // Generate timing information
    if (config_.include_timing_info) {
        auto base_time = std::chrono::steady_clock::now();
        generate_timing_info(frame, frame_number, base_time);
    }
    
    // Generate FIC data
    if (config_.include_fic_data) {
        generate_fic_data(frame, frame_number);
    }
    
    // Generate MSC data
    if (config_.include_msc_data) {
        generate_msc_data(frame, frame_number);
    }
    
    // Update CRC to maintain frame integrity
    frame.update_crc();
    
    frames_generated_++;
    return frame;
}

std::vector<EtsiEtiFrame> EtiFrameGenerator::generate_sequence(
    size_t frame_count,
    std::chrono::steady_clock::time_point start_time) {
    
    std::vector<EtsiEtiFrame> sequence;
    sequence.reserve(frame_count);
    
    for (size_t i = 0; i < frame_count; ++i) {
        EtsiEtiFrame frame = generate_frame(static_cast<uint32_t>(i));
        
        // Update timing for sequence consistency
        if (config_.precise_timing) {
            auto frame_time = start_time + (config_.frame_interval * i);
            auto tist_us = std::chrono::duration_cast<std::chrono::microseconds>(
                frame_time.time_since_epoch()).count();
            
            uint32_t tist = static_cast<uint32_t>(tist_us & 0xFFFFFFFF);
            frame.data[8] = (tist >> 24) & 0xFF;
            frame.data[9] = (tist >> 16) & 0xFF;
            frame.data[10] = (tist >> 8) & 0xFF;
            frame.data[11] = tist & 0xFF;
            
            frame.update_crc();
        }
        
        sequence.push_back(frame);
    }
    
    return sequence;
}

std::vector<EtsiEtiFrame> EtiFrameGenerator::generate_with_services(
    const std::vector<AudioServiceConfig>& service_configs,
    size_t frame_count) {
    
    std::vector<EtsiEtiFrame> frames;
    frames.reserve(frame_count);
    
    // Update generator config based on service configurations
    GeneratorConfig temp_config = config_;
    temp_config.service_count = static_cast<uint8_t>(service_configs.size());
    
    for (size_t i = 0; i < frame_count; ++i) {
        EtsiEtiFrame frame;
        frame.set_frame_count(static_cast<uint8_t>(i % 250));
        frame.set_sub_channel_count(temp_config.service_count);
        frame.set_ensemble_id(temp_config.ensemble_id);
        
        // Generate service-specific content
        uint8_t* msc = &frame.data[44];
        size_t msc_offset = 0;
        
        for (const auto& service : service_configs) {
            // Calculate service data size based on bit rate
            size_t service_size = (service.bit_rate * 1000) / (8 * 250); // Bytes per frame
            
            if (msc_offset + service_size <= 6096) {
                // Generate service-specific pattern
                uint8_t pattern_base = static_cast<uint8_t>(service.service_id & 0xFF);
                
                if (service.is_dab_plus) {
                    // DAB+ pattern (HE-AAC)
                    msc[msc_offset] = 0x56; // AAC sync
                    msc[msc_offset + 1] = 0xE0;
                    for (size_t j = 2; j < service_size; ++j) {
                        msc[msc_offset + j] = static_cast<uint8_t>((pattern_base + j + i) & 0xFF);
                    }
                } else {
                    // DAB pattern (MPEG-1 Layer 2)
                    msc[msc_offset] = 0xFF; // MPEG sync
                    msc[msc_offset + 1] = 0xFD;
                    for (size_t j = 2; j < service_size; ++j) {
                        msc[msc_offset + j] = static_cast<uint8_t>((pattern_base + j + i) & 0xFF);
                    }
                }
                
                msc_offset += service_size;
            }
        }
        
        // Fill remaining MSC with padding
        while (msc_offset < 6096) {
            msc[msc_offset++] = 0x55; // Padding pattern
        }
        
        frame.update_crc();
        frames.push_back(frame);
    }
    
    frames_generated_ += frame_count;
    return frames;
}

void EtiFrameGenerator::generate_fic_data(EtsiEtiFrame& frame, uint32_t frame_number) {
    uint8_t* fic = &frame.data[12];
    
    // FIG 0/0: Ensemble information
    fic[0] = 0x00;  // FIG type 0/0
    fic[1] = 0x05;  // Length
    fic[2] = (config_.ensemble_id >> 8) & 0xFF;
    fic[3] = config_.ensemble_id & 0xFF;
    fic[4] = 0x00;  // Change flags
    fic[5] = (frame_number >> 8) & 0xFF;  // CIF count (high)
    fic[6] = frame_number & 0xFF;         // CIF count (low)
    
    // FIG 0/1: Sub-channel organization
    fic[7] = 0x01;  // FIG type 0/1
    fic[8] = static_cast<uint8_t>(4 * config_.service_count); // Length
    
    size_t fic_offset = 9;
    uint16_t start_address = 0;
    
    for (uint8_t i = 0; i < config_.service_count && fic_offset < 32; ++i) {
        if (fic_offset + 4 <= 32) {
            fic[fic_offset++] = i; // Sub-channel ID
            fic[fic_offset++] = (start_address >> 8) & 0xFF;
            fic[fic_offset++] = start_address & 0xFF;
            fic[fic_offset++] = 0x40; // Size = 64 CUs, Protection level 2
            
            start_address += 64; // Next service starts after this one
        }
    }
    
    // Fill remaining FIC with padding
    while (fic_offset < 32) {
        fic[fic_offset++] = 0xFF;
    }
}

void EtiFrameGenerator::generate_msc_data(EtsiEtiFrame& frame, uint32_t frame_number) {
    uint8_t* msc = &frame.data[44];
    
    if (config_.realistic_audio_patterns) {
        // Generate realistic audio patterns for each service
        std::uniform_int_distribution<uint8_t> audio_dist(0x80, 0xBF); // Audio-like range
        
        size_t service_size = 6096 / config_.service_count;
        
        for (uint8_t service = 0; service < config_.service_count; ++service) {
            size_t service_offset = service * service_size;
            
            // Generate service-specific audio pattern
            uint8_t pattern_base = static_cast<uint8_t>(0xA0 + service);
            
            for (size_t i = 0; i < service_size && service_offset + i < 6096; ++i) {
                // Create pseudo-audio pattern with frame dependency
                uint8_t value = static_cast<uint8_t>(
                    (pattern_base + (frame_number & 0xFF) + (i & 0xFF)) & 0xFF
                );
                
                // Add some noise for realism
                if (rng_() % 100 < 10) { // 10% noise
                    value ^= audio_dist(rng_);
                }
                
                msc[service_offset + i] = value;
            }
        }
    } else {
        // Simple pattern generation
        for (size_t i = 0; i < 6096; ++i) {
            msc[i] = static_cast<uint8_t>((frame_number + i) & 0xFF);
        }
    }
}

void EtiFrameGenerator::generate_timing_info(
    EtsiEtiFrame& frame, 
    uint32_t frame_number, 
    std::chrono::steady_clock::time_point base_time) {
    
    // Calculate TIST based on frame number and base time
    auto frame_time = base_time + (config_.frame_interval * frame_number);
    auto tist_us = std::chrono::duration_cast<std::chrono::microseconds>(
        frame_time.time_since_epoch()).count();
    
    uint32_t tist = static_cast<uint32_t>(tist_us & 0xFFFFFFFF);
    
    // Set TIST in LIDATA field (bytes 8-11)
    frame.data[8] = (tist >> 24) & 0xFF;
    frame.data[9] = (tist >> 16) & 0xFF;
    frame.data[10] = (tist >> 8) & 0xFF;
    frame.data[11] = tist & 0xFF;
}

void EtiFrameGenerator::update_config(const GeneratorConfig& new_config) {
    config_ = new_config;
}

// ErrorInjectionGenerator implementation
ErrorInjectionGenerator::ErrorInjectionGenerator(const ErrorConfig& config)
    : config_(config), rng_(std::random_device{}()) {
    
    // Initialize default error weights if not provided
    if (config_.error_weights.empty()) {
        config_.error_weights.resize(static_cast<size_t>(ErrorType::RANDOM_CORRUPTION) + 1, 1.0);
    }
}

EtsiEtiFrame ErrorInjectionGenerator::inject_errors(const EtsiEtiFrame& clean_frame) {
    EtsiEtiFrame corrupted_frame = clean_frame;
    
    // Decide whether to inject errors based on error rate
    std::uniform_real_distribution<double> rate_dist(0.0, 1.0);
    if (rate_dist(rng_) > config_.error_rate) {
        return corrupted_frame; // No error injection
    }
    
    // Select error type(s) to inject
    std::vector<ErrorType> errors_to_inject;
    
    if (config_.enabled_errors.empty()) {
        // Random error selection
        errors_to_inject.push_back(select_random_error_type());
    } else {
        // Use configured error types
        for (ErrorType error_type : config_.enabled_errors) {
            if (should_inject_error(error_type)) {
                errors_to_inject.push_back(error_type);
            }
        }
    }
    
    // Inject selected errors
    for (ErrorType error_type : errors_to_inject) {
        switch (error_type) {
            case ErrorType::SYNC_CORRUPTION:
                inject_sync_corruption(corrupted_frame);
                break;
            case ErrorType::HEADER_CORRUPTION:
                inject_header_corruption(corrupted_frame);
                break;
            case ErrorType::FIC_CORRUPTION:
                inject_fic_corruption(corrupted_frame);
                break;
            case ErrorType::MSC_CORRUPTION:
                inject_msc_corruption(corrupted_frame);
                break;
            case ErrorType::CRC_CORRUPTION:
                inject_crc_corruption(corrupted_frame);
                break;
            case ErrorType::TIMING_ERROR:
                inject_timing_error(corrupted_frame);
                break;
            case ErrorType::RANDOM_CORRUPTION:
                inject_random_corruption(corrupted_frame, 8); // 8 random bit flips
                break;
            default:
                break;
        }
        
        statistics_.error_counts[error_type]++;
    }
    
    if (!errors_to_inject.empty()) {
        statistics_.frames_with_errors++;
    }
    
    statistics_.total_frames_processed++;
    statistics_.actual_error_rate = 
        static_cast<double>(statistics_.frames_with_errors) / statistics_.total_frames_processed;
    
    return corrupted_frame;
}

std::vector<EtsiEtiFrame> ErrorInjectionGenerator::inject_sequence_errors(
    const std::vector<EtsiEtiFrame>& clean_frames) {
    
    std::vector<EtsiEtiFrame> corrupted_sequence;
    corrupted_sequence.reserve(clean_frames.size());
    
    if (config_.cluster_errors) {
        // Inject clustered errors
        std::uniform_int_distribution<size_t> cluster_start_dist(0, clean_frames.size() - 1);
        size_t cluster_start = cluster_start_dist(rng_);
        
        for (size_t i = 0; i < clean_frames.size(); ++i) {
            if (i >= cluster_start && i < cluster_start + config_.cluster_size) {
                // Force error injection in cluster
                EtsiEtiFrame corrupted = clean_frames[i];
                ErrorType error_type = select_random_error_type();
                
                switch (error_type) {
                    case ErrorType::SYNC_CORRUPTION:
                        inject_sync_corruption(corrupted);
                        break;
                    case ErrorType::FIC_CORRUPTION:
                        inject_fic_corruption(corrupted);
                        break;
                    default:
                        inject_random_corruption(corrupted, 4);
                        break;
                }
                
                corrupted_sequence.push_back(corrupted);
                statistics_.frames_with_errors++;
            } else {
                corrupted_sequence.push_back(clean_frames[i]);
            }
            
            statistics_.total_frames_processed++;
        }
    } else {
        // Independent error injection per frame
        for (const auto& frame : clean_frames) {
            corrupted_sequence.push_back(inject_errors(frame));
        }
    }
    
    return corrupted_sequence;
}

std::vector<EtsiEtiFrame> ErrorInjectionGenerator::generate_error_scenario(
    ErrorType scenario_type,
    const std::vector<EtsiEtiFrame>& base_frames) {
    
    std::vector<EtsiEtiFrame> scenario_frames;
    scenario_frames.reserve(base_frames.size());
    
    for (size_t i = 0; i < base_frames.size(); ++i) {
        EtsiEtiFrame frame = base_frames[i];
        
        switch (scenario_type) {
            case ErrorType::SEQUENCE_DISRUPTION: {
                // Inject frame sequence errors
                if (i % 10 == 0) { // Every 10th frame
                    frame.data[4] = static_cast<uint8_t>((i + 50) % 250); // Wrong frame count
                }
                break;
            }
            
            case ErrorType::CONTENT_MISMATCH: {
                // Inject content inconsistencies
                if (i % 20 == 0) { // Every 20th frame
                    frame.data[5] = 0xFF; // Invalid NST
                }
                break;
            }
            
            case ErrorType::BUFFER_OVERFLOW: {
                // Simulate buffer overflow conditions
                if (i > base_frames.size() / 2) { // Second half of stream
                    inject_random_corruption(frame, 16); // Heavy corruption
                }
                break;
            }
            
            default:
                // Apply single error type consistently
                switch (scenario_type) {
                    case ErrorType::SYNC_CORRUPTION:
                        inject_sync_corruption(frame);
                        break;
                    case ErrorType::CRC_CORRUPTION:
                        inject_crc_corruption(frame);
                        break;
                    default:
                        break;
                }
                break;
        }
        
        scenario_frames.push_back(frame);
    }
    
    return scenario_frames;
}

void ErrorInjectionGenerator::inject_sync_corruption(EtsiEtiFrame& frame) {
    // Corrupt sync pattern
    std::uniform_int_distribution<uint8_t> byte_dist(0, 255);
    
    frame.data[0] = byte_dist(rng_);
    frame.data[1] = byte_dist(rng_);
    frame.data[2] = byte_dist(rng_);
    frame.data[3] = byte_dist(rng_);
}

void ErrorInjectionGenerator::inject_header_corruption(EtsiEtiFrame& frame) {
    // Corrupt LIDATA field
    std::uniform_int_distribution<size_t> offset_dist(4, 11);
    std::uniform_int_distribution<uint8_t> byte_dist(0, 255);
    
    size_t corrupt_offset = offset_dist(rng_);
    frame.data[corrupt_offset] = byte_dist(rng_);
}

void ErrorInjectionGenerator::inject_fic_corruption(EtsiEtiFrame& frame) {
    // Corrupt FIC data
    std::uniform_int_distribution<size_t> offset_dist(12, 43);
    std::uniform_int_distribution<uint8_t> byte_dist(0, 255);
    std::uniform_int_distribution<size_t> count_dist(1, 8);
    
    size_t corrupt_count = count_dist(rng_);
    for (size_t i = 0; i < corrupt_count; ++i) {
        size_t corrupt_offset = offset_dist(rng_);
        frame.data[corrupt_offset] = byte_dist(rng_);
    }
}

void ErrorInjectionGenerator::inject_msc_corruption(EtsiEtiFrame& frame) {
    // Corrupt MSC data
    std::uniform_int_distribution<size_t> offset_dist(44, 6139);
    std::uniform_int_distribution<uint8_t> byte_dist(0, 255);
    std::uniform_int_distribution<size_t> count_dist(1, 32);
    
    size_t corrupt_count = count_dist(rng_);
    for (size_t i = 0; i < corrupt_count; ++i) {
        size_t corrupt_offset = offset_dist(rng_);
        frame.data[corrupt_offset] = byte_dist(rng_);
    }
}

void ErrorInjectionGenerator::inject_crc_corruption(EtsiEtiFrame& frame) {
    // Corrupt CRC field
    std::uniform_int_distribution<uint8_t> byte_dist(0, 255);
    
    frame.data[6140] = byte_dist(rng_);
    frame.data[6141] = byte_dist(rng_);
    frame.data[6142] = byte_dist(rng_);
    frame.data[6143] = byte_dist(rng_);
}

void ErrorInjectionGenerator::inject_timing_error(EtsiEtiFrame& frame) {
    // Inject timing errors in TIST field
    std::uniform_int_distribution<uint32_t> tist_dist(0, 0xFFFFFFFF);
    
    uint32_t bad_tist = tist_dist(rng_);
    frame.data[8] = (bad_tist >> 24) & 0xFF;
    frame.data[9] = (bad_tist >> 16) & 0xFF;
    frame.data[10] = (bad_tist >> 8) & 0xFF;
    frame.data[11] = bad_tist & 0xFF;
}

void ErrorInjectionGenerator::inject_random_corruption(EtsiEtiFrame& frame, size_t bit_count) {
    std::uniform_int_distribution<size_t> byte_offset_dist(0, 6143);
    std::uniform_int_distribution<uint8_t> bit_offset_dist(0, 7);
    
    for (size_t i = 0; i < bit_count; ++i) {
        size_t byte_offset = byte_offset_dist(rng_);
        uint8_t bit_offset = bit_offset_dist(rng_);
        
        frame.data[byte_offset] ^= (1 << bit_offset); // Flip bit
    }
}

bool ErrorInjectionGenerator::should_inject_error(ErrorType type) {
    std::uniform_real_distribution<double> prob_dist(0.0, 1.0);
    
    // Use weighted probability if available
    size_t type_index = static_cast<size_t>(type);
    if (type_index < config_.error_weights.size()) {
        return prob_dist(rng_) < (config_.error_rate * config_.error_weights[type_index]);
    }
    
    return prob_dist(rng_) < config_.error_rate;
}

ErrorInjectionGenerator::ErrorType ErrorInjectionGenerator::select_random_error_type() {
    std::uniform_int_distribution<int> type_dist(
        0, static_cast<int>(ErrorType::RANDOM_CORRUPTION)
    );
    
    return static_cast<ErrorType>(type_dist(rng_));
}

void ErrorInjectionGenerator::reset_statistics() {
    statistics_ = ErrorStatistics{};
}

void ErrorInjectionGenerator::update_config(const ErrorConfig& new_config) {
    config_ = new_config;
}

// Performance Test Generator Implementation
PerformanceTestGenerator::PerformanceTestGenerator(const PerformanceConfig& config)
    : config_(config), rng_(std::random_device{}()) {
}

std::vector<EtsiEtiFrame> PerformanceTestGenerator::generate_performance_test_data() {
    size_t total_frames = config_.target_fps * config_.test_duration_seconds;
    size_t warmup_frames = config_.target_fps * config_.warmup_seconds;
    
    std::vector<EtsiEtiFrame> test_data;
    test_data.reserve(total_frames + warmup_frames);
    
    // Generate warmup frames
    for (size_t i = 0; i < warmup_frames; ++i) {
        test_data.push_back(generate_constant_pattern_frame(static_cast<uint32_t>(i)));
    }
    
    // Generate main test frames
    for (size_t i = warmup_frames; i < total_frames + warmup_frames; ++i) {
        EtsiEtiFrame frame;
        
        switch (config_.pattern) {
            case PerformanceConfig::DataPattern::CONSTANT:
                frame = generate_constant_pattern_frame(static_cast<uint32_t>(i));
                break;
            case PerformanceConfig::DataPattern::RANDOM:
                frame = generate_random_pattern_frame(static_cast<uint32_t>(i));
                break;
            case PerformanceConfig::DataPattern::WORST_CASE:
                frame = generate_worst_case_frame(static_cast<uint32_t>(i));
                break;
            case PerformanceConfig::DataPattern::REALISTIC:
                frame = generate_realistic_frame(static_cast<uint32_t>(i));
                break;
            case PerformanceConfig::DataPattern::MIXED:
                frame = generate_mixed_pattern_frame(static_cast<uint32_t>(i));
                break;
        }
        
        test_data.push_back(frame);
    }
    
    return test_data;
}

EtsiEtiFrame PerformanceTestGenerator::generate_constant_pattern_frame(uint32_t frame_number) {
    EtsiEtiFrame frame = GERMAN_PUBLIC_RADIO_ENSEMBLE;
    frame.set_frame_count(static_cast<uint8_t>(frame_number % 250));
    
    // Fill MSC with constant pattern for predictable processing
    uint8_t* msc = &frame.data[44];
    std::fill(msc, msc + 6096, 0xAA);
    
    frame.update_crc();
    return frame;
}

EtsiEtiFrame PerformanceTestGenerator::generate_random_pattern_frame(uint32_t frame_number) {
    EtsiEtiFrame frame = GERMAN_PUBLIC_RADIO_ENSEMBLE;
    frame.set_frame_count(static_cast<uint8_t>(frame_number % 250));
    
    // Fill MSC with random data for unpredictable processing load
    uint8_t* msc = &frame.data[44];
    for (size_t i = 0; i < 6096; ++i) {
        msc[i] = static_cast<uint8_t>(rng_() & 0xFF);
    }
    
    frame.update_crc();
    return frame;
}

EtsiEtiFrame PerformanceTestGenerator::generate_worst_case_frame(uint32_t frame_number) {
    EtsiEtiFrame frame = MAXIMUM_CAPACITY_ENSEMBLE;
    frame.set_frame_count(static_cast<uint8_t>(frame_number % 250));
    
    // Create worst-case processing scenario
    // Maximum services, complex patterns, high entropy data
    uint8_t* msc = &frame.data[44];
    
    // High-entropy pattern that's difficult to compress/optimize
    for (size_t i = 0; i < 6096; ++i) {
        msc[i] = static_cast<uint8_t>((i * 31 + frame_number * 17 + rng_()) & 0xFF);
    }
    
    frame.update_crc();
    return frame;
}

EtsiEtiFrame PerformanceTestGenerator::generate_realistic_frame(uint32_t frame_number) {
    EtsiEtiFrame frame = GERMAN_PUBLIC_RADIO_ENSEMBLE;
    frame.set_frame_count(static_cast<uint8_t>(frame_number % 250));
    
    // Generate realistic broadcast content patterns
    uint8_t* msc = &frame.data[44];
    
    // Simulate audio content with typical patterns
    std::normal_distribution<double> audio_dist(128.0, 32.0);
    
    for (size_t i = 0; i < 6096; ++i) {
        double sample = audio_dist(rng_);
        sample = std::max(0.0, std::min(255.0, sample));
        msc[i] = static_cast<uint8_t>(sample);
    }
    
    frame.update_crc();
    return frame;
}

EtsiEtiFrame PerformanceTestGenerator::generate_mixed_pattern_frame(uint32_t frame_number) {
    // Alternate between different patterns
    switch (frame_number % 4) {
        case 0: return generate_constant_pattern_frame(frame_number);
        case 1: return generate_random_pattern_frame(frame_number);
        case 2: return generate_worst_case_frame(frame_number);
        default: return generate_realistic_frame(frame_number);
    }
}

// Utility functions implementation
namespace TestDataUtils {

bool save_test_data(
    const std::vector<EtsiEtiFrame>& frames,
    const std::string& filename,
    const std::string& format) {
    
    try {
        if (format == "binary") {
            std::ofstream file(filename, std::ios::binary);
            if (!file) return false;
            
            // Write frame count
            uint32_t frame_count = static_cast<uint32_t>(frames.size());
            file.write(reinterpret_cast<const char*>(&frame_count), sizeof(frame_count));
            
            // Write frames
            for (const auto& frame : frames) {
                file.write(reinterpret_cast<const char*>(frame.data.data()), frame.data.size());
            }
            
            return file.good();
        }
        // Other formats can be implemented as needed
        return false;
    } catch (const std::exception&) {
        return false;
    }
}

std::vector<EtsiEtiFrame> load_test_data(
    const std::string& filename,
    const std::string& format) {
    
    std::vector<EtsiEtiFrame> frames;
    
    try {
        if (format == "binary") {
            std::ifstream file(filename, std::ios::binary);
            if (!file) return frames;
            
            // Read frame count
            uint32_t frame_count;
            file.read(reinterpret_cast<char*>(&frame_count), sizeof(frame_count));
            if (!file.good()) return frames;
            
            frames.reserve(frame_count);
            
            // Read frames
            for (uint32_t i = 0; i < frame_count; ++i) {
                EtsiEtiFrame frame;
                file.read(reinterpret_cast<char*>(frame.data.data()), frame.data.size());
                if (!file.good()) break;
                frames.push_back(frame);
            }
        }
        // Other formats can be implemented as needed
    } catch (const std::exception&) {
        frames.clear();
    }
    
    return frames;
}

std::vector<std::string> validate_test_data(const std::vector<EtsiEtiFrame>& frames) {
    std::vector<std::string> report;
    
    report.push_back("TEST DATA VALIDATION REPORT");
    report.push_back("===========================");
    report.push_back("Total frames: " + std::to_string(frames.size()));
    
    size_t valid_frames = 0;
    size_t sync_errors = 0;
    size_t crc_errors = 0;
    size_t structure_errors = 0;
    
    for (size_t i = 0; i < frames.size(); ++i) {
        const auto& frame = frames[i];
        
        bool is_valid = true;
        
        if (!frame.validate_sync_pattern()) {
            sync_errors++;
            is_valid = false;
        }
        
        if (!frame.validate_frame_structure()) {
            structure_errors++;
            is_valid = false;
        }
        
        if (!frame.validate_crc()) {
            crc_errors++;
            is_valid = false;
        }
        
        if (is_valid) {
            valid_frames++;
        }
    }
    
    report.push_back("Valid frames: " + std::to_string(valid_frames));
    report.push_back("Sync errors: " + std::to_string(sync_errors));
    report.push_back("CRC errors: " + std::to_string(crc_errors));
    report.push_back("Structure errors: " + std::to_string(structure_errors));
    
    double validity_percentage = frames.empty() ? 0.0 : 
        (static_cast<double>(valid_frames) / frames.size()) * 100.0;
    
    report.push_back("Validity: " + std::to_string(validity_percentage) + "%");
    
    if (validity_percentage >= 95.0) {
        report.push_back("Result: EXCELLENT");
    } else if (validity_percentage >= 90.0) {
        report.push_back("Result: GOOD");
    } else if (validity_percentage >= 80.0) {
        report.push_back("Result: ACCEPTABLE");
    } else {
        report.push_back("Result: POOR - Review test data generation");
    }
    
    return report;
}

} // namespace TestDataUtils

} // namespace TestDataGenerators