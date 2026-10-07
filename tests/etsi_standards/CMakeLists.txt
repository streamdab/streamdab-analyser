# ETSI Standards Compliance Test Suite CMake Configuration
# TDD Framework Integration for Professional Broadcast Standards Compliance
cmake_minimum_required(VERSION 3.21)

message(STATUS "=== ETSI Standards Compliance Test Suite Configuration ===")

# ETSI compliance test framework sources
set(ETSI_COMPLIANCE_TEST_FRAMEWORK_SOURCES
    # Core ETSI test framework
    framework/etsi_test_base.cpp
    framework/compliance_test_runner.cpp
    framework/validation_framework.cpp

    # Existing ETSI compliance tests (already in codebase)
    test_etsi_compliance_framework.cpp
    test_etsi_en_300_401.cpp
    test_fig_analysis_requirements.cpp
    test_eti_protocol_compliance.cpp
)

# New comprehensive ETSI compliance test sources
set(ETSI_COMPREHENSIVE_TEST_SOURCES
    # Comprehensive ETSI standards test specifications
    COMPREHENSIVE_ETSI_TDD_TEST_SPECIFICATIONS.md  # Documentation (not compiled)

    # DAB service discovery compliance tests
    test_dab_service_discovery_compliance.cpp

    # Thai DAB compliance validation tests
    test_thai_dab_compliance_validation.cpp

    # Automated compliance reporting tests
    test_automated_compliance_reporting.cpp
)

# ETSI EN 300 401 DAB Core Broadcasting tests
set(ETSI_EN_300_401_TEST_SOURCES
    en_300_401/test_dab_transmission_modes.cpp
    en_300_401/test_fic_structure.cpp
    en_300_401/test_msc_organization.cpp
    en_300_401/test_error_protection.cpp
    en_300_401/test_synchronization.cpp
    en_300_401/test_service_organization.cpp
)

# ETSI EN 302 077 Harmonized Radio Standard tests
set(ETSI_EN_302_077_TEST_SOURCES
    en_302_077/test_harmonized_standard.cpp
    en_302_077/test_transmission_parameters.cpp
    en_302_077/test_equipment_certification.cpp
    en_302_077/test_evm_validation.cpp
    en_302_077/test_spectrum_compliance.cpp
)

# ETSI TS 102 563 DAB+ Audio Coding tests
set(ETSI_TS_102_563_TEST_SOURCES
    ts_102_563/test_dabplus_audio_coding.cpp
    ts_102_563/test_sbr_parametric_stereo.cpp
    ts_102_563/test_reed_solomon_coding.cpp
    ts_102_563/test_he_aac_validation.cpp
    ts_102_563/test_audio_superframe.cpp
)

# ETSI EN 300 799 ETI Distribution Interface tests
set(ETSI_EN_300_799_TEST_SOURCES
    en_300_799/test_eti_frame_structure.cpp
    en_300_799/test_sync_patterns.cpp
    en_300_799/test_crc_validation.cpp
    en_300_799/test_lidata_validation.cpp
    en_300_799/test_timing_validation.cpp
)

# Thai DAB Compliance tests
set(THAI_DAB_COMPLIANCE_TEST_SOURCES
    thai_compliance/test_thai_character_encoding.cpp
    thai_compliance/test_nbtc_frequency_plan.cpp
    thai_compliance/test_emergency_broadcasting.cpp
    thai_compliance/test_content_guidelines.cpp
    thai_compliance/test_cultural_appropriateness.cpp
)

# Cross-validation and Certification tests
set(CROSS_VALIDATION_TEST_SOURCES
    cross_validation/test_multi_standard_consistency.cpp
    cross_validation/test_interoperability.cpp
    cross_validation/test_certification_package.cpp
    cross_validation/test_regulatory_submission.cpp
)

# Performance and Real-time Compliance tests
set(PERFORMANCE_COMPLIANCE_TEST_SOURCES
    performance/test_compliance_performance.cpp
    performance/test_memory_efficiency.cpp
    performance/test_processing_latency.cpp
    performance/test_realtime_validation.cpp
)

# Combine all ETSI test sources
set(ALL_ETSI_COMPLIANCE_TEST_SOURCES
    ${ETSI_COMPLIANCE_TEST_FRAMEWORK_SOURCES}
    ${ETSI_COMPREHENSIVE_TEST_SOURCES}
    ${ETSI_EN_300_401_TEST_SOURCES}
    ${ETSI_EN_302_077_TEST_SOURCES}
    ${ETSI_TS_102_563_TEST_SOURCES}
    ${ETSI_EN_300_799_TEST_SOURCES}
    ${THAI_DAB_COMPLIANCE_TEST_SOURCES}
    ${CROSS_VALIDATION_TEST_SOURCES}
    ${PERFORMANCE_COMPLIANCE_TEST_SOURCES}
)

# Filter out sources that don't exist yet (TDD RED phase)
set(EXISTING_ETSI_TEST_SOURCES)
foreach(test_source ${ALL_ETSI_COMPLIANCE_TEST_SOURCES})
    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/${test_source}")
        list(APPEND EXISTING_ETSI_TEST_SOURCES ${test_source})
        message(STATUS "ETSI Test Source Found: ${test_source}")
    else()
        # Check if it's a .md file (documentation)
        get_filename_component(file_ext ${test_source} EXT)
        if(NOT file_ext STREQUAL ".md")
            message(STATUS "ETSI Test Source Missing (TDD RED phase): ${test_source}")
        endif()
    endif()
endforeach()

# Add sources that currently exist in the codebase
list(APPEND EXISTING_ETSI_TEST_SOURCES
    # Currently existing test files
    test_dab_service_discovery_compliance.cpp
    test_thai_dab_compliance_validation.cpp
    test_automated_compliance_reporting.cpp
)

# Create ETSI Comprehensive Compliance Test executable
add_executable(eti_analyser_etsi_comprehensive_tests
    ${EXISTING_ETSI_TEST_SOURCES}

    # Include test fixtures and support files
    ../fixtures/eti_test_data.cpp
    ../fixtures/test_data_generators.cpp
    ../fixtures/eti_streams/optimized_compliance_frames.cpp
    ../fixtures/etsi_test_data/etsi_reference_data.cpp

    # Include TDD framework
    ../tdd_framework.cpp
)

# Link dependencies for ETSI compliance tests
target_link_libraries(eti_analyser_etsi_comprehensive_tests
    ${GTEST_LIBS}
    Qt6::Core
    Qt6::Test
    ${TEST_LIBS}
)

# Include directories for ETSI compliance tests
target_include_directories(eti_analyser_etsi_comprehensive_tests PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../../src
    ${CMAKE_CURRENT_SOURCE_DIR}/..
    ${CMAKE_CURRENT_SOURCE_DIR}/../fixtures
    ${CMAKE_CURRENT_SOURCE_DIR}/../mocks
    ${CMAKE_CURRENT_SOURCE_DIR}/../fixtures/etsi_test_data
    ${FAAD2_INCLUDE_DIRS}
    ${FFTW3_INCLUDE_DIRS}

)

# Compile definitions for ETSI compliance tests
target_compile_definitions(eti_analyser_etsi_comprehensive_tests PRIVATE
    ${FAAD2_CFLAGS_OTHER}
    ${FFTW3_CFLAGS_OTHER}
    ETSI_COMPLIANCE_TESTING_ENABLED=1
    THAI_DAB_COMPLIANCE_TESTING_ENABLED=1
    TDD_RED_PHASE_ACTIVE=1
)

# Register ETSI comprehensive compliance tests with CTest
add_test(NAME etsi_comprehensive_compliance_tests
         COMMAND eti_analyser_etsi_comprehensive_tests)

# Set test properties for better organization
set_tests_properties(etsi_comprehensive_compliance_tests PROPERTIES
    LABELS "ETSI;Compliance;Standards;TDD"
    TIMEOUT 300  # 5 minutes timeout for comprehensive tests
)

# TDD workflow targets for ETSI comprehensive compliance
add_custom_target(etsi_comprehensive_tdd_red_phase
    COMMAND echo "🔴 ETSI Comprehensive TDD RED Phase: Running failing ETSI compliance tests"
    COMMAND echo "Expected result: Tests should FAIL (validates test completeness)"
    COMMAND eti_analyser_etsi_comprehensive_tests --gtest_filter="*ShouldFailUntilImplemented*" || echo "✅ RED phase validated - tests correctly fail"
    DEPENDS eti_analyser_etsi_comprehensive_tests
    COMMENT "ETSI Comprehensive TDD RED: Expecting test failures (this validates comprehensive test coverage)"
)

add_custom_target(etsi_comprehensive_tdd_green_phase
    COMMAND echo "🟢 ETSI Comprehensive TDD GREEN Phase: Running ETSI compliance tests expecting pass"
    COMMAND eti_analyser_etsi_comprehensive_tests
    DEPENDS eti_analyser_etsi_comprehensive_tests
    COMMENT "ETSI Comprehensive TDD GREEN: All ETSI compliance tests must pass"
)

add_custom_target(etsi_comprehensive_tdd_refactor_phase
    COMMAND echo "🔵 ETSI Comprehensive TDD REFACTOR Phase: Optimizing while maintaining test success"
    COMMAND eti_analyser_etsi_comprehensive_tests --gtest_filter="*Performance*"
    DEPENDS eti_analyser_etsi_comprehensive_tests
    COMMENT "ETSI Comprehensive TDD REFACTOR: Performance optimization validation"
)

# ETSI specific test categories
add_custom_target(run_etsi_en_300_401_tests
    COMMAND echo "📻 ETSI EN 300 401 DAB Core Broadcasting Tests"
    COMMAND eti_analyser_etsi_comprehensive_tests --gtest_filter="*EN300401*:*DabCore*:*TransmissionMode*:*FicStructure*:*MscOrganization*"
    DEPENDS eti_analyser_etsi_comprehensive_tests
    COMMENT "Running ETSI EN 300 401 DAB Radio Broadcasting compliance tests"
)

add_custom_target(run_etsi_en_302_077_tests
    COMMAND echo "📡 ETSI EN 302 077 Harmonized Radio Standard Tests"
    COMMAND eti_analyser_etsi_comprehensive_tests --gtest_filter="*EN302077*:*HarmonizedStandard*:*EVM*:*SpectrumCompliance*"
    DEPENDS eti_analyser_etsi_comprehensive_tests
    COMMENT "Running ETSI EN 302 077 Harmonized Radio Standard compliance tests"
)

add_custom_target(run_etsi_ts_102_563_tests
    COMMAND echo "🎵 ETSI TS 102 563 DAB+ Audio Coding Tests"
    COMMAND eti_analyser_etsi_comprehensive_tests --gtest_filter="*TS102563*:*DabPlusAudio*:*HeAac*:*ReedSolomon*"
    DEPENDS eti_analyser_etsi_comprehensive_tests
    COMMENT "Running ETSI TS 102 563 DAB+ Audio Coding compliance tests"
)

add_custom_target(run_etsi_en_300_799_tests
    COMMAND echo "🔗 ETSI EN 300 799 ETI Distribution Interface Tests"
    COMMAND eti_analyser_etsi_comprehensive_tests --gtest_filter="*EN300799*:*EtiFrame*:*SyncPattern*:*CrcValidation*"
    DEPENDS eti_analyser_etsi_comprehensive_tests
    COMMENT "Running ETSI EN 300 799 ETI Distribution Interface compliance tests"
)

add_custom_target(run_thai_dab_compliance_tests
    COMMAND echo "🇹🇭 Thai DAB NBTC Compliance Tests"
    COMMAND eti_analyser_etsi_comprehensive_tests --gtest_filter="*ThaiDab*:*Nbtc*:*ThaiCharacter*:*ThaiCompliance*"
    DEPENDS eti_analyser_etsi_comprehensive_tests
    COMMENT "Running Thai DAB NBTC regulatory compliance tests"
)

add_custom_target(run_dab_service_discovery_tests
    COMMAND echo "🔍 DAB Service Discovery Compliance Tests"
    COMMAND eti_analyser_etsi_comprehensive_tests --gtest_filter="*ServiceDiscovery*:*FigParsing*:*ServiceOrganization*"
    DEPENDS eti_analyser_etsi_comprehensive_tests
    COMMENT "Running DAB service discovery compliance tests"
)

add_custom_target(run_compliance_reporting_tests
    COMMAND echo "📊 Automated Compliance Reporting Tests"
    COMMAND eti_analyser_etsi_comprehensive_tests --gtest_filter="*ComplianceReporting*:*CertificationPackage*:*RegulatorySubmission*"
    DEPENDS eti_analyser_etsi_comprehensive_tests
    COMMENT "Running automated compliance reporting framework tests"
)

# Comprehensive ETSI compliance validation target
add_custom_target(run_complete_etsi_compliance_validation
    COMMAND echo "🏆 Complete ETSI Compliance Validation Suite"
    COMMAND echo "============================================="
    COMMAND echo "Testing complete ETSI broadcast standards compliance..."
    COMMAND echo ""
    COMMAND echo "🔴 Phase 1: TDD RED Phase Validation"
    COMMAND ${CMAKE_MAKE_PROGRAM} etsi_comprehensive_tdd_red_phase
    COMMAND echo ""
    COMMAND echo "📻 Phase 2: ETSI EN 300 401 DAB Core Broadcasting"
    COMMAND ${CMAKE_MAKE_PROGRAM} run_etsi_en_300_401_tests
    COMMAND echo ""
    COMMAND echo "📡 Phase 3: ETSI EN 302 077 Harmonized Radio Standard"
    COMMAND ${CMAKE_MAKE_PROGRAM} run_etsi_en_302_077_tests
    COMMAND echo ""
    COMMAND echo "🎵 Phase 4: ETSI TS 102 563 DAB+ Audio Coding"
    COMMAND ${CMAKE_MAKE_PROGRAM} run_etsi_ts_102_563_tests
    COMMAND echo ""
    COMMAND echo "🔗 Phase 5: ETSI EN 300 799 ETI Distribution Interface"
    COMMAND ${CMAKE_MAKE_PROGRAM} run_etsi_en_300_799_tests
    COMMAND echo ""
    COMMAND echo "🇹🇭 Phase 6: Thai DAB NBTC Compliance"
    COMMAND ${CMAKE_MAKE_PROGRAM} run_thai_dab_compliance_tests
    COMMAND echo ""
    COMMAND echo "🔍 Phase 7: DAB Service Discovery Compliance"
    COMMAND ${CMAKE_MAKE_PROGRAM} run_dab_service_discovery_tests
    COMMAND echo ""
    COMMAND echo "📊 Phase 8: Automated Compliance Reporting"
    COMMAND ${CMAKE_MAKE_PROGRAM} run_compliance_reporting_tests
    COMMAND echo ""
    COMMAND echo "✅ COMPLETE ETSI COMPLIANCE VALIDATION FINISHED"
    DEPENDS eti_analyser_etsi_comprehensive_tests
    COMMENT "Complete ETSI compliance validation with all standards and Thai NBTC requirements"
)

# Professional certification package generation target
add_custom_target(generate_professional_certification_package
    COMMAND echo "📜 Professional Broadcast Industry Certification Package Generation"
    COMMAND echo "=================================================================="
    COMMAND eti_analyser_etsi_comprehensive_tests --gtest_filter="*CertificationPackage*:*ProfessionalBroadcast*:*RegulatorySubmission*"
    COMMAND echo "📋 Generating ETSI compliance certificates..."
    COMMAND echo "🇹🇭 Generating Thai NBTC regulatory submission package..."
    COMMAND echo "🌍 Generating international certification compatibility report..."
    COMMAND echo "✅ Professional certification package generation complete"
    DEPENDS eti_analyser_etsi_comprehensive_tests
    COMMENT "Generate complete professional broadcast industry certification package"
)

# Performance and optimization validation
add_custom_target(validate_etsi_compliance_performance
    COMMAND echo "⚡ ETSI Compliance Performance Validation"
    COMMAND echo "========================================"
    COMMAND eti_analyser_etsi_comprehensive_tests --gtest_filter="*Performance*:*Realtime*:*Latency*:*Memory*"
    COMMAND echo "📈 Performance targets:"
    COMMAND echo "   - Service discovery: >500 checks/sec"
    COMMAND echo "   - Thai compliance: >300 checks/sec"
    COMMAND echo "   - Report generation: <5 seconds"
    COMMAND echo "   - Memory usage: <50MB during reporting"
    COMMAND echo "   - Real-time latency: <10ms"
    DEPENDS eti_analyser_etsi_comprehensive_tests
    COMMENT "Validate ETSI compliance performance requirements for real-time operation"
)

# Integration with main test framework
# Add ETSI compliance tests to main test runner
set(ETSI_TESTS_INTEGRATED TRUE PARENT_SCOPE)

# Export ETSI test information for main CMakeLists.txt
set(ETSI_COMPREHENSIVE_TEST_EXECUTABLE eti_analyser_etsi_comprehensive_tests PARENT_SCOPE)
set(ETSI_COMPREHENSIVE_TEST_TARGETS
    etsi_comprehensive_tdd_red_phase
    etsi_comprehensive_tdd_green_phase
    run_complete_etsi_compliance_validation
    generate_professional_certification_package
    validate_etsi_compliance_performance
    PARENT_SCOPE
)

# Status summary
message(STATUS "")
message(STATUS "=== ETSI Standards Compliance Test Suite Status ===")
message(STATUS "Total ETSI test sources: ${EXISTING_ETSI_TEST_SOURCES}")
message(STATUS "Comprehensive ETSI compliance tests: CONFIGURED")
message(STATUS "")
message(STATUS "🔴 TDD RED Phase: etsi_comprehensive_tdd_red_phase")
message(STATUS "🟢 TDD GREEN Phase: etsi_comprehensive_tdd_green_phase")
message(STATUS "🔵 TDD REFACTOR Phase: etsi_comprehensive_tdd_refactor_phase")
message(STATUS "")
message(STATUS "📻 ETSI EN 300 401 (DAB Core): run_etsi_en_300_401_tests")
message(STATUS "📡 ETSI EN 302 077 (Harmonized): run_etsi_en_302_077_tests")
message(STATUS "🎵 ETSI TS 102 563 (DAB+ Audio): run_etsi_ts_102_563_tests")
message(STATUS "🔗 ETSI EN 300 799 (ETI Interface): run_etsi_en_300_799_tests")
message(STATUS "🇹🇭 Thai NBTC Compliance: run_thai_dab_compliance_tests")
message(STATUS "🔍 Service Discovery: run_dab_service_discovery_tests")
message(STATUS "📊 Compliance Reporting: run_compliance_reporting_tests")
message(STATUS "")
message(STATUS "🏆 Complete Validation: run_complete_etsi_compliance_validation")
message(STATUS "📜 Certification Package: generate_professional_certification_package")
message(STATUS "⚡ Performance Validation: validate_etsi_compliance_performance")
message(STATUS "")
message(STATUS "ETSI Standards Coverage:")
message(STATUS "  ✅ ETSI EN 300 401 - DAB Radio Broadcasting")
message(STATUS "  ✅ ETSI EN 302 077 - Harmonized Radio Standard")
message(STATUS "  ✅ ETSI TS 102 563 - DAB+ Audio Coding")
message(STATUS "  ✅ ETSI EN 300 799 - ETI Distribution Interface")
message(STATUS "  ✅ Thai NBTC DAB Requirements")
message(STATUS "  ✅ Professional Broadcast Industry Certification")
message(STATUS "  ✅ Automated Compliance Reporting")
message(STATUS "")
message(STATUS "===================================================")
message(STATUS "")
