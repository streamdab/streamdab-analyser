/**
 * @file etsi_reference_data.h
 * @brief ETSI Reference Test Data and Validation Criteria
 * 
 * This file provides comprehensive ETSI-compliant test data, validation criteria,
 * and reference implementations for TDD development. All data conforms to
 * ETSI standards and provides known-good test vectors for validation.
 * 
 * Standards compliance:
 * - ETSI EN 300 401 (DAB Radio Broadcasting)
 * - ETSI EN 302 077 (DAB+ Audio Coding)
 * - ETSI EN 300 799 (ETI Distribution Interface)
 * - ETSI TS 102 563 (DAB+ Guidelines)
 * - ETSI TS 101 756 (Registered Tables)
 */

#pragma once

#include <cstdint>
#include <vector>
#include <string>
#include <map>
#include <chrono>
#include <array>

/**
 * @namespace EtsiReferenceData
 * @brief Namespace containing ETSI-compliant reference test data
 */
namespace EtsiReferenceData {

/**
 * @brief ETSI EN 300 799 compliant ETI frame structures
 */
namespace EtiFrameData {

    /**
     * @brief Standard ETI frame constants (ETSI EN 300 799)
     */
    struct EtiConstants {
        static constexpr size_t FRAME_SIZE = 6144;           // Total ETI frame size
        static constexpr size_t SYNC_SIZE = 4;               // Synchronization field
        static constexpr size_t LIDATA_SIZE = 8;             // LIDATA field
        static constexpr size_t FIC_SIZE = 32;               // Fast Information Channel
        static constexpr size_t MSC_SIZE = 6096;             // Main Service Channel
        static constexpr size_t CRC_SIZE = 4;                // CRC field
        static constexpr std::chrono::milliseconds FRAME_DURATION{24};  // 24ms per frame
        
        // Sync pattern (ETSI EN 300 799 Section 5.1)
        static constexpr std::array<uint8_t, 4> SYNC_PATTERN = {0x49, 0x93, 0x1E, 0x03};
    };

    /**
     * @brief Complete ETSI-compliant ETI frame with valid structure
     * 
     * This frame contains:
     * - Valid sync pattern
     * - Proper LIDATA field configuration
     * - Complete FIC with essential FIGs
     * - MSC with DAB+ audio service
     * - Correct CRC-16 checksum
     */
    extern const std::array<uint8_t, EtiConstants::FRAME_SIZE> REFERENCE_COMPLIANT_FRAME;

    /**
     * @brief ETI frame with multiple DAB+ services
     * 
     * Contains 3 DAB+ audio services for comprehensive testing:
     * - Service 1: Stereo 128 kbps HE-AAC v2
     * - Service 2: Mono 64 kbps HE-AAC v2  
     * - Service 3: Stereo 96 kbps HE-AAC v2
     */
    extern const std::array<uint8_t, EtiConstants::FRAME_SIZE> MULTI_SERVICE_FRAME;

    /**
     * @brief ETI frame sequence for continuity testing
     * 
     * 250 consecutive frames (0-249) for frame count wraparound testing
     */
    class ContinuityTestSequence {
    public:
        static constexpr size_t SEQUENCE_LENGTH = 250;
        static std::vector<std::array<uint8_t, EtiConstants::FRAME_SIZE>> generate_sequence();
        static bool validate_frame_continuity(const std::vector<std::array<uint8_t, EtiConstants::FRAME_SIZE>>& frames);
    };

} // namespace EtiFrameData

/**
 * @brief ETSI EN 300 401 DAB transmission mode test data
 */
namespace TransmissionModeData {

    /**
     * @brief Mode I transmission parameters (most common)
     */
    struct ModeIParameters {
        static constexpr uint32_t CARRIERS = 1536;
        static constexpr uint32_t SYMBOL_DURATION_US = 1000;
        static constexpr uint32_t NULL_SYMBOL_DURATION_US = 1297;
        static constexpr uint32_t GUARD_INTERVAL_US = 246;
        static constexpr uint32_t SYMBOLS_PER_FRAME = 76;  // 1 null + 3 phase ref + 72 data
        static constexpr double SAMPLING_RATE_KHZ = 2048.0;
    };

    /**
     * @brief Mode II transmission parameters
     */
    struct ModeIIParameters {
        static constexpr uint32_t CARRIERS = 384;
        static constexpr uint32_t SYMBOL_DURATION_US = 250;
        static constexpr uint32_t NULL_SYMBOL_DURATION_US = 324;
        static constexpr uint32_t GUARD_INTERVAL_US = 62;
        static constexpr uint32_t SYMBOLS_PER_FRAME = 76;
        static constexpr double SAMPLING_RATE_KHZ = 2048.0;
    };

    /**
     * @brief OFDM symbol test data for validation
     */
    class OfdmSymbolTestData {
    public:
        // Null symbol for synchronization (Mode I)
        static std::vector<std::complex<float>> generate_null_symbol_mode_i();
        
        // Phase reference symbol (Mode I)
        static std::vector<std::complex<float>> generate_phase_reference_symbol_mode_i();
        
        // Data symbol with known content (Mode I)
        static std::vector<std::complex<float>> generate_data_symbol_mode_i(uint32_t symbol_index);
    };

} // namespace TransmissionModeData

/**
 * @brief FIC/FIG test data (ETSI EN 300 401 Section 5.2)
 */
namespace FigTestData {

    /**
     * @brief FIG 0/0 Ensemble Information test data
     */
    struct Fig0_0_TestData {
        uint16_t ensemble_id = 0x1234;
        uint8_t change_flags = 0x00;
        bool alarm_flag = false;
        uint16_t cif_count = 0x1A5;
        uint8_t occurrence_change = 0x00;
        
        // Binary representation of FIG 0/0
        static const std::vector<uint8_t> BINARY_DATA;
    };

    /**
     * @brief FIG 0/1 Sub-channel Organization test data
     */
    struct Fig0_1_TestData {
        struct SubChannel {
            uint8_t subchannel_id;
            uint16_t start_address;
            uint8_t table_switch;
            uint8_t table_index;
            uint16_t subchannel_size;
            uint8_t protection_level;
            bool uep_flag;
        };
        
        static const std::vector<SubChannel> TEST_SUBCHANNELS;
        static const std::vector<uint8_t> BINARY_DATA;
    };

    /**
     * @brief FIG 0/2 Service Organization test data
     */
    struct Fig0_2_TestData {
        struct Service {
            uint16_t service_id;
            uint8_t country_id;
            uint8_t extended_country_code;
            bool programme_service;
            std::vector<uint8_t> component_ids;
        };
        
        static const std::vector<Service> TEST_SERVICES;
        static const std::vector<uint8_t> BINARY_DATA;
    };

    /**
     * @brief FIG 1/0 Ensemble Label test data
     */
    struct Fig1_0_TestData {
        uint16_t ensemble_id = 0x1234;
        std::string label = "DAB Test Ensemble";  // 16 characters padded
        uint16_t character_flag = 0x0000;
        
        static const std::vector<uint8_t> BINARY_DATA;
    };

    /**
     * @brief FIG 1/1 Service Label test data
     */
    struct Fig1_1_TestData {
        struct ServiceLabel {
            uint16_t service_id;
            std::string label;  // 16 characters
            uint16_t character_flag;
        };
        
        static const std::vector<ServiceLabel> TEST_SERVICE_LABELS;
        static const std::vector<uint8_t> BINARY_DATA;
    };

    /**
     * @brief Complete FIC block with multiple FIGs
     */
    class CompleteFicBlock {
    public:
        static std::array<uint8_t, 32> generate_complete_fic();
        static bool validate_fic_structure(const std::array<uint8_t, 32>& fic_data);
        static std::vector<uint8_t> extract_fig_blocks(const std::array<uint8_t, 32>& fic_data);
    };

} // namespace FigTestData

/**
 * @brief DAB+ audio test data (ETSI EN 302 077)
 */
namespace DabPlusAudioData {

    /**
     * @brief HE-AAC v2 audio parameters
     */
    struct HeAacV2Parameters {
        static constexpr uint32_t SAMPLING_RATE_HZ = 48000;
        static constexpr uint8_t SUPERFRAME_SIZE_MS = 120;
        static constexpr uint8_t RS_OUTER_PARITY_BYTES = 10;
        static constexpr uint8_t FIRECODE_INNER_PARITY_BYTES = 16;
        static constexpr uint8_t MAX_BITRATE_KBPS = 256;
    };

    /**
     * @brief Reed-Solomon outer coding test vectors
     */
    class ReedSolomonTestData {
    public:
        static constexpr uint8_t CODE_LENGTH = 204;
        static constexpr uint8_t DATA_LENGTH = 188;
        static constexpr uint8_t PARITY_LENGTH = 16;
        
        // Known-good RS encoded block
        static const std::array<uint8_t, CODE_LENGTH> VALID_RS_BLOCK;
        
        // RS block with single correctable error
        static const std::array<uint8_t, CODE_LENGTH> SINGLE_ERROR_BLOCK;
        
        // RS block with multiple correctable errors
        static const std::array<uint8_t, CODE_LENGTH> MULTI_ERROR_BLOCK;
        
        // RS block with uncorrectable errors
        static const std::array<uint8_t, CODE_LENGTH> UNCORRECTABLE_BLOCK;
    };

    /**
     * @brief FireCode inner coding test vectors
     */
    class FireCodeTestData {
    public:
        static constexpr uint8_t CODEWORD_LENGTH = 32;
        static constexpr uint8_t DATA_LENGTH = 24;
        static constexpr uint8_t PARITY_LENGTH = 8;
        
        // Known-good FireCode encoded block
        static const std::array<uint8_t, CODEWORD_LENGTH> VALID_FIRECODE_BLOCK;
        
        // FireCode block with correctable error
        static const std::array<uint8_t, CODEWORD_LENGTH> CORRECTABLE_ERROR_BLOCK;
    };

    /**
     * @brief Complete DAB+ audio superframe
     */
    class AudioSuperframeTestData {
    public:
        // Stereo 128 kbps HE-AAC v2 superframe
        static std::vector<uint8_t> generate_stereo_128kbps_superframe();
        
        // Mono 64 kbps HE-AAC v2 superframe
        static std::vector<uint8_t> generate_mono_64kbps_superframe();
        
        // Validate superframe structure
        static bool validate_superframe_structure(const std::vector<uint8_t>& superframe_data);
    };

} // namespace DabPlusAudioData

/**
 * @brief Character encoding test data (ETSI TS 101 756)
 */
namespace CharacterEncodingData {

    /**
     * @brief Complete EBU Latin character set test strings
     */
    struct EbuLatinTestStrings {
        static const std::vector<std::string> BASIC_LATIN_STRINGS;
        static const std::vector<std::string> EXTENDED_LATIN_STRINGS;
        static const std::vector<std::string> SPECIAL_CHARACTERS;
    };

    /**
     * @brief UTF-8 encoding test strings
     */
    struct Utf8TestStrings {
        static const std::vector<std::string> UNICODE_STRINGS;
        static const std::vector<std::string> MULTILINGUAL_STRINGS;
        static const std::vector<std::string> EMOJI_STRINGS;
    };

    /**
     * @brief Language code test data (ISO 639)
     */
    struct LanguageCodeTestData {
        struct LanguageCode {
            std::string iso_639_1;  // 2-letter code
            std::string iso_639_2;  // 3-letter code
            std::string language_name;
        };
        
        static const std::vector<LanguageCode> COMMON_LANGUAGE_CODES;
        static const std::vector<std::string> INVALID_LANGUAGE_CODES;
    };

} // namespace CharacterEncodingData

/**
 * @brief Performance test data and benchmarks
 */
namespace PerformanceTestData {

    /**
     * @brief Performance benchmark targets
     */
    struct PerformanceBenchmarks {
        static constexpr uint32_t MIN_ETI_FRAMES_PER_SECOND = 900;
        static constexpr uint32_t MIN_FIG_PARSING_PER_SECOND = 1000;
        static constexpr uint32_t MIN_COMPLIANCE_CHECKS_PER_SECOND = 500;
        static constexpr std::chrono::milliseconds MAX_FRAME_PROCESSING_LATENCY{5};
        static constexpr std::chrono::microseconds MAX_FIG_PARSING_LATENCY{100};
        static constexpr std::chrono::milliseconds MAX_COMPLIANCE_CHECK_LATENCY{10};
        static constexpr double MAX_CPU_USAGE_PERCENT = 25.0;
        static constexpr size_t MAX_MEMORY_USAGE_MB = 150;
    };

    /**
     * @brief Large-scale test data generators
     */
    class LargeScaleTestData {
    public:
        // Generate continuous ETI stream for performance testing
        static std::vector<std::array<uint8_t, EtiFrameData::EtiConstants::FRAME_SIZE>> 
            generate_continuous_stream(size_t frame_count);
        
        // Generate stream with known error patterns
        static std::vector<std::array<uint8_t, EtiFrameData::EtiConstants::FRAME_SIZE>>
            generate_error_stream(size_t frame_count, double error_rate);
        
        // Generate memory stress test data
        static std::vector<uint8_t> generate_memory_stress_data(size_t size_mb);
    };

} // namespace PerformanceTestData

/**
 * @brief Error test cases and edge conditions
 */
namespace ErrorTestCases {

    /**
     * @brief Common error scenarios for robustness testing
     */
    enum class ErrorType {
        INVALID_SYNC_PATTERN,
        CORRUPTED_LIDATA,
        INVALID_FIC_CRC,
        TRUNCATED_FRAME,
        OVERSIZED_FRAME,
        CORRUPTED_MSC_DATA,
        INVALID_FIG_STRUCTURE,
        REED_SOLOMON_ERRORS,
        AUDIO_CODEC_ERRORS,
        CHARACTER_ENCODING_ERRORS
    };

    /**
     * @brief Error injection utilities
     */
    class ErrorInjection {
    public:
        // Inject specific error type into ETI frame
        static std::array<uint8_t, EtiFrameData::EtiConstants::FRAME_SIZE>
            inject_error(const std::array<uint8_t, EtiFrameData::EtiConstants::FRAME_SIZE>& frame,
                        ErrorType error_type);
        
        // Generate frame with multiple error types
        static std::array<uint8_t, EtiFrameData::EtiConstants::FRAME_SIZE>
            generate_multi_error_frame(const std::vector<ErrorType>& error_types);
        
        // Validate error detection capability
        static bool validate_error_detection(const std::array<uint8_t, EtiFrameData::EtiConstants::FRAME_SIZE>& frame,
                                            ErrorType expected_error);
    };

} // namespace ErrorTestCases

/**
 * @brief Validation criteria and quality metrics
 */
namespace ValidationCriteria {

    /**
     * @brief ETSI compliance validation thresholds
     */
    struct ComplianceThresholds {
        static constexpr double MIN_FRAME_COMPLIANCE_PERCENT = 99.0;
        static constexpr double MIN_FIG_COMPLIANCE_PERCENT = 98.0;
        static constexpr double MIN_AUDIO_COMPLIANCE_PERCENT = 99.5;
        static constexpr uint32_t MAX_CORRECTABLE_ERRORS_PER_FRAME = 3;
        static constexpr uint32_t MAX_UNCORRECTABLE_ERRORS_PER_HOUR = 10;
    };

    /**
     * @brief Quality assessment metrics
     */
    struct QualityMetrics {
        double frame_sync_accuracy;
        double timing_accuracy;
        double audio_quality_score;
        double ensemble_integrity_score;
        uint32_t total_errors_detected;
        uint32_t errors_corrected;
        std::chrono::milliseconds average_processing_time;
    };

    /**
     * @brief Validation result aggregation
     */
    class ValidationResultAggregator {
    public:
        struct ValidationSummary {
            bool overall_compliance;
            std::map<std::string, bool> standard_compliance;
            QualityMetrics quality_metrics;
            std::vector<std::string> violations;
            std::vector<std::string> warnings;
            std::vector<std::string> recommendations;
        };
        
        static ValidationSummary aggregate_results(const std::vector<QualityMetrics>& results);
        static bool meets_broadcast_quality_standards(const ValidationSummary& summary);
        static std::string generate_compliance_report(const ValidationSummary& summary);
    };

} // namespace ValidationCriteria

/**
 * @brief Test data factory for automated test generation
 */
class EtsiTestDataFactory {
public:
    /**
     * @brief Generate test data sets for specific testing scenarios
     */
    enum class TestScenario {
        BASIC_COMPLIANCE,
        PERFORMANCE_STRESS,
        ERROR_RECOVERY,
        MULTI_SERVICE,
        CONTINUOUS_OPERATION,
        CROSS_STANDARD_VALIDATION
    };
    
    struct TestDataSet {
        std::vector<std::array<uint8_t, EtiFrameData::EtiConstants::FRAME_SIZE>> eti_frames;
        std::vector<std::string> expected_services;
        std::vector<std::string> expected_labels;
        ValidationCriteria::ValidationSummary expected_results;
        std::string scenario_description;
    };
    
    static TestDataSet generate_test_data(TestScenario scenario, size_t data_size = 1000);
    static std::vector<TestDataSet> generate_comprehensive_test_suite();
    static bool validate_test_data_integrity(const TestDataSet& test_data);
};

} // namespace EtsiReferenceData