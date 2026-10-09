# TDD Workflow Enforcement CMake Module
# Implements Red-Green-Refactor workflow with quality gates

cmake_minimum_required(VERSION 3.21)

# TDD Phase detection and enforcement
function(tdd_enforce_workflow)
    # Check if TDD environment variables are set
    if(DEFINED ENV{TDD_PHASE})
        set(TDD_CURRENT_PHASE $ENV{TDD_PHASE})
        message(STATUS "TDD Phase: ${TDD_CURRENT_PHASE}")

        if(TDD_CURRENT_PHASE STREQUAL "RED")
            message(STATUS "🔴 TDD RED Phase: Writing failing tests first")
            add_compile_definitions(TDD_FORCE_RED=1)
        elseif(TDD_CURRENT_PHASE STREQUAL "GREEN")
            message(STATUS "🟢 TDD GREEN Phase: Implementing minimal code to pass")
            add_compile_definitions(TDD_FORCE_GREEN=1)
        elseif(TDD_CURRENT_PHASE STREQUAL "REFACTOR")
            message(STATUS "🔵 TDD REFACTOR Phase: Improving code while keeping tests green")
            add_compile_definitions(TDD_FORCE_REFACTOR=1)
        endif()
    endif()
endfunction()

# Coverage enforcement function
function(tdd_enforce_coverage TARGET MINIMUM_PERCENTAGE)
    if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
        # Add coverage flags to target
        target_compile_options(${TARGET} PRIVATE --coverage -fprofile-arcs -ftest-coverage)
        target_link_options(${TARGET} PRIVATE --coverage)

        # Create coverage target
        add_custom_target(${TARGET}_coverage
            COMMAND ${CMAKE_CTEST_COMMAND} --test-dir ${CMAKE_BINARY_DIR}
            COMMAND lcov --capture --directory . --output-file ${TARGET}_coverage.info
            COMMAND lcov --remove ${TARGET}_coverage.info '/usr/*' '*/tests/*' '*/third_party/*' --output-file ${TARGET}_coverage_clean.info
            COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/../scripts/check_coverage.sh
            DEPENDS ${TARGET}
            WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
            COMMENT "Generating coverage report for ${TARGET} (minimum ${MINIMUM_PERCENTAGE}%)"
        )

        # Set coverage requirement
        set_property(TARGET ${TARGET}_coverage PROPERTY MINIMUM_COVERAGE ${MINIMUM_PERCENTAGE})

        message(STATUS "Coverage enforcement enabled for ${TARGET}: ${MINIMUM_PERCENTAGE}% minimum")
    else()
        message(WARNING "Coverage enforcement not available for ${CMAKE_CXX_COMPILER_ID}")
    endif()
endfunction()

# TDD test discovery and registration
function(tdd_register_test TEST_TARGET TEST_CATEGORY PRIORITY)
    # Add test to CTest
    add_test(NAME ${TEST_TARGET} COMMAND ${TEST_TARGET})

    # Set test properties based on category and priority
    set_property(TEST ${TEST_TARGET} PROPERTY LABELS ${TEST_CATEGORY})

    # Set test timeout based on category
    if(TEST_CATEGORY STREQUAL "UNIT")
        set_property(TEST ${TEST_TARGET} PROPERTY TIMEOUT 30)
    elseif(TEST_CATEGORY STREQUAL "INTEGRATION")
        set_property(TEST ${TEST_TARGET} PROPERTY TIMEOUT 60)
    elseif(TEST_CATEGORY STREQUAL "E2E")
        set_property(TEST ${TEST_TARGET} PROPERTY TIMEOUT 120)
    elseif(TEST_CATEGORY STREQUAL "GUI")
        set_property(TEST ${TEST_TARGET} PROPERTY TIMEOUT 90)
    elseif(TEST_CATEGORY STREQUAL "PERFORMANCE")
        set_property(TEST ${TEST_TARGET} PROPERTY TIMEOUT 300)
    else()
        set_property(TEST ${TEST_TARGET} PROPERTY TIMEOUT 60)
    endif()

    # Set test priority
    if(PRIORITY STREQUAL "CRITICAL")
        set_property(TEST ${TEST_TARGET} PROPERTY PRIORITY 1)
    elseif(PRIORITY STREQUAL "HIGH")
        set_property(TEST ${TEST_TARGET} PROPERTY PRIORITY 2)
    elseif(PRIORITY STREQUAL "MEDIUM")
        set_property(TEST ${TEST_TARGET} PROPERTY PRIORITY 3)
    else()
        set_property(TEST ${TEST_TARGET} PROPERTY PRIORITY 4)
    endif()

    message(STATUS "Registered TDD test: ${TEST_TARGET} [${TEST_CATEGORY}/${PRIORITY}]")
endfunction()

# Quality gate enforcement
function(tdd_create_quality_gate GATE_NAME)
    set(QUALITY_GATE_TESTS ${ARGN})

    # Create quality gate target that runs specific tests
    add_custom_target(${GATE_NAME}
        COMMAND echo "🚦 Quality Gate: ${GATE_NAME}"
        COMMAND echo "Running quality gate tests..."
        COMMENT "TDD Quality Gate: ${GATE_NAME}"
    )

    # Add test execution for each test in the gate
    foreach(TEST_TARGET ${QUALITY_GATE_TESTS})
        add_custom_command(TARGET ${GATE_NAME} POST_BUILD
            COMMAND ${CMAKE_CTEST_COMMAND} --output-on-failure -R ${TEST_TARGET}
            COMMENT "Running ${TEST_TARGET} for quality gate ${GATE_NAME}"
        )
    endforeach()

    message(STATUS "Created quality gate: ${GATE_NAME} with ${CMAKE_ARGC} tests")
endfunction()

# TDD compliance check
function(tdd_create_compliance_check)
    add_custom_target(tdd_compliance_check
        COMMAND echo "🔍 TDD Compliance Check"
        COMMAND echo "========================"
        COMMAND ${CMAKE_CTEST_COMMAND} --output-on-failure
        COMMAND echo "📊 Generating compliance report..."
        COMMAND ${CMAKE_COMMAND} -E env TDD_COMPLIANCE_CHECK=1
                ${CMAKE_CURRENT_SOURCE_DIR}/scripts/check_coverage.sh
        DEPENDS run_tests
        COMMENT "Comprehensive TDD compliance validation"
    )

    # Create separate targets for different compliance levels
    add_custom_target(tdd_basic_compliance
        COMMAND ${CMAKE_CTEST_COMMAND} --output-on-failure -L "UNIT|CRITICAL"
        COMMENT "Basic TDD compliance (unit tests + critical functionality)"
    )

    add_custom_target(tdd_full_compliance
        COMMAND ${CMAKE_CTEST_COMMAND} --output-on-failure
        COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/scripts/check_coverage.sh
        COMMENT "Full TDD compliance (all tests + coverage requirements)"
    )
endfunction()

# Performance benchmark integration
function(tdd_add_performance_benchmark TARGET BENCHMARK_TARGET MAX_DURATION_MS)
    add_custom_target(${TARGET}_benchmark
        COMMAND echo "⏱️  Performance Benchmark: ${TARGET}"
        COMMAND echo "Maximum allowed duration: ${MAX_DURATION_MS}ms"
        COMMAND timeout ${MAX_DURATION_MS}ms ${BENCHMARK_TARGET} ||
                (echo "❌ Performance benchmark failed: exceeded ${MAX_DURATION_MS}ms" && exit 1)
        DEPENDS ${BENCHMARK_TARGET}
        COMMENT "Performance benchmark for ${TARGET}"
    )

    # Register as test
    add_test(NAME ${TARGET}_performance COMMAND ${CMAKE_COMMAND} --build ${CMAKE_BINARY_DIR} --target ${TARGET}_benchmark)
    set_property(TEST ${TARGET}_performance PROPERTY LABELS "PERFORMANCE")
    set_property(TEST ${TARGET}_performance PROPERTY TIMEOUT $((${MAX_DURATION_MS} / 1000 + 10)))
endfunction()

# Memory leak detection (Linux/macOS with Valgrind)
function(tdd_add_memory_check TARGET)
    find_program(VALGRIND_EXECUTABLE valgrind)
    if(VALGRIND_EXECUTABLE AND UNIX AND NOT APPLE)
        add_custom_target(${TARGET}_memcheck
            COMMAND ${VALGRIND_EXECUTABLE}
                --tool=memcheck
                --leak-check=full
                --show-reachable=yes
                --num-callers=20
                --track-fds=yes
                --error-exitcode=1
                $<TARGET_FILE:${TARGET}>
            DEPENDS ${TARGET}
            COMMENT "Memory leak check for ${TARGET}"
        )

        # Register as test
        add_test(NAME ${TARGET}_memory COMMAND ${CMAKE_COMMAND} --build ${CMAKE_BINARY_DIR} --target ${TARGET}_memcheck)
        set_property(TEST ${TARGET}_memory PROPERTY LABELS "MEMORY")
        set_property(TEST ${TARGET}_memory PROPERTY TIMEOUT 300)

        message(STATUS "Memory check enabled for ${TARGET}")
    endif()
endfunction()

# ETSI compliance validation
function(tdd_add_etsi_compliance_test TARGET STANDARD)
    add_custom_target(${TARGET}_etsi_compliance
        COMMAND echo "📋 ETSI Compliance Test: ${STANDARD}"
        COMMAND ${TARGET} --gtest_filter="*ETSI*:*Compliance*"
        DEPENDS ${TARGET}
        COMMENT "ETSI ${STANDARD} compliance validation"
    )

    # Register as test
    add_test(NAME ${TARGET}_etsi_${STANDARD} COMMAND ${CMAKE_COMMAND} --build ${CMAKE_BINARY_DIR} --target ${TARGET}_etsi_compliance)
    set_property(TEST ${TARGET}_etsi_${STANDARD} PROPERTY LABELS "ETSI")
    set_property(TEST ${TARGET}_etsi_${STANDARD} PROPERTY TIMEOUT 120)
endfunction()

# Initialize TDD workflow
tdd_enforce_workflow()

message(STATUS "TDD Workflow CMake module loaded")
message(STATUS "Available TDD targets:")
message(STATUS "  - tdd_red_phase: Run tests in RED phase")
message(STATUS "  - tdd_green_phase: Run tests in GREEN phase")
message(STATUS "  - tdd_compliance_check: Full compliance validation")
message(STATUS "  - *_coverage: Generate coverage reports")
message(STATUS "  - *_benchmark: Performance benchmarks")
message(STATUS "  - *_memcheck: Memory leak detection")
message(STATUS "  - *_etsi_compliance: ETSI standard validation")
