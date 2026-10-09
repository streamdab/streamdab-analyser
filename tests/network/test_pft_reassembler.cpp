/**
 * @file test_pft_reassembler.cpp
 * @brief Unit tests for PFT Packet Reassembler
 *
 * Tests ETSI TS 102 693 compliant PFT packet reassembly implementation.
 *
 * Test coverage:
 * - Single fragment reassembly
 * - Multiple fragments in order
 * - Out-of-order fragment handling
 * - Duplicate fragment detection
 * - Missing fragment detection
 * - Timeout cleanup
 * - Statistics tracking
 * - FEC recovery attempts
 *
 * @author Backend Developer
 * @date 2025-10-19
 * @copyright Copyright (c) 2025 StreamDAB Analyser Team
 */

#include <gtest/gtest.h>
#include "../../src/network/pft_reassembler.hpp"
#include "../../include/edi_types.hpp"
#include <thread>
#include <chrono>

using namespace edi;

/**
 * @brief Test fixture for PFT Reassembler tests
 */
class PFTReassemblerTest : public ::testing::Test {
protected:
    void SetUp() override {
        reassembler = std::make_unique<PFTReassembler>();
    }

    void TearDown() override {
        reassembler.reset();
    }

    /**
     * @brief Create a test PFT fragment
     * @param pseq Protocol sequence number
     * @param findex Fragment index
     * @param fcount Total fragment count
     * @param payload_size Size of fragment payload
     * @return PFT packet fragment
     */
    PFTPacket createFragment(uint16_t pseq, uint16_t findex, uint16_t fcount, size_t payload_size = 100) {
        PFTPacket fragment;
        fragment.sync_pattern = constants::PFT_SYNC_PATTERN_1;
        fragment.pseq = pseq;
        fragment.findex = findex;
        fragment.fcount = fcount;
        fragment.fec_type = 0;  // No FEC
        fragment.crc = 0;

        // Create dummy payload with identifiable pattern
        fragment.payload.resize(payload_size);
        for (size_t i = 0; i < payload_size; ++i) {
            fragment.payload[i] = static_cast<uint8_t>((findex * 256 + i) % 256);
        }

        fragment.received_time = std::chrono::system_clock::now();

        return fragment;
    }

    /**
     * @brief Create fragments for a complete sequence
     * @param pseq Protocol sequence number
     * @param fcount Total fragment count
     * @param fragment_size Size of each fragment
     * @return Vector of PFT fragments
     */
    std::vector<PFTPacket> createCompleteSequence(uint16_t pseq, uint16_t fcount, size_t fragment_size = 100) {
        std::vector<PFTPacket> fragments;
        fragments.reserve(fcount);

        for (uint16_t i = 0; i < fcount; ++i) {
            fragments.push_back(createFragment(pseq, i, fcount, fragment_size));
        }

        return fragments;
    }

    std::unique_ptr<PFTReassembler> reassembler;
};

// ============================================================================
// Basic Functionality Tests
// ============================================================================

/**
 * @brief Test single fragment reassembly (fcount=1)
 */
TEST_F(PFTReassemblerTest, SingleFragmentReassembly) {
    // Create single fragment packet
    auto fragment = createFragment(100, 0, 1, 256);

    // Add fragment
    ASSERT_TRUE(reassembler->addFragment(fragment));

    // Check if complete
    EXPECT_TRUE(reassembler->isComplete(100));

    // Get reassembled packet
    auto reassembled = reassembler->getReassembledPacket(100);
    ASSERT_TRUE(reassembled.has_value());

    // Verify payload
    EXPECT_EQ(reassembled->size(), 256);
    EXPECT_EQ(*reassembled, fragment.payload);

    // Verify statistics
    auto stats = reassembler->getStatistics();
    EXPECT_EQ(stats.total_fragments, 1);
    EXPECT_EQ(stats.complete_sequences, 1);
    EXPECT_EQ(stats.lost_sequences, 0);
}

/**
 * @brief Test multiple fragments received in order
 */
TEST_F(PFTReassemblerTest, MultipleFragmentsInOrder) {
    const uint16_t pseq = 200;
    const uint16_t fcount = 5;
    const size_t fragment_size = 100;

    // Create and add fragments in order
    auto fragments = createCompleteSequence(pseq, fcount, fragment_size);

    for (const auto& fragment : fragments) {
        ASSERT_TRUE(reassembler->addFragment(fragment));
    }

    // Check if complete
    EXPECT_TRUE(reassembler->isComplete(pseq));

    // Get reassembled packet
    auto reassembled = reassembler->getReassembledPacket(pseq);
    ASSERT_TRUE(reassembled.has_value());

    // Verify total size
    EXPECT_EQ(reassembled->size(), fcount * fragment_size);

    // Verify payload order
    for (uint16_t i = 0; i < fcount; ++i) {
        const auto& expected_fragment = fragments[i];
        auto start = reassembled->begin() + (i * fragment_size);
        auto end = start + fragment_size;

        std::vector<uint8_t> actual_fragment(start, end);
        EXPECT_EQ(actual_fragment, expected_fragment.payload)
            << "Fragment " << i << " mismatch";
    }

    // Verify statistics
    auto stats = reassembler->getStatistics();
    EXPECT_EQ(stats.total_fragments, fcount);
    EXPECT_EQ(stats.complete_sequences, 1);
    EXPECT_EQ(stats.out_of_order_fragments, 0);
}

/**
 * @brief Test out-of-order fragment reception
 */
TEST_F(PFTReassemblerTest, OutOfOrderFragments) {
    const uint16_t pseq = 300;
    const uint16_t fcount = 4;

    auto fragments = createCompleteSequence(pseq, fcount);

    // Add fragments out of order: 0, 2, 1, 3
    ASSERT_TRUE(reassembler->addFragment(fragments[0]));
    ASSERT_TRUE(reassembler->addFragment(fragments[2]));
    ASSERT_TRUE(reassembler->addFragment(fragments[1]));
    ASSERT_TRUE(reassembler->addFragment(fragments[3]));

    // Should still complete successfully
    EXPECT_TRUE(reassembler->isComplete(pseq));

    // Verify reassembly
    auto reassembled = reassembler->getReassembledPacket(pseq);
    ASSERT_TRUE(reassembled.has_value());

    // Verify statistics
    auto stats = reassembler->getStatistics();
    EXPECT_EQ(stats.total_fragments, fcount);
    EXPECT_EQ(stats.complete_sequences, 1);
    EXPECT_GT(stats.out_of_order_fragments, 0);  // Should have detected out-of-order
}

/**
 * @brief Test duplicate fragment detection
 */
TEST_F(PFTReassemblerTest, DuplicateFragmentDetection) {
    const uint16_t pseq = 400;
    auto fragment = createFragment(pseq, 0, 3);

    // Add fragment first time
    ASSERT_TRUE(reassembler->addFragment(fragment));

    // Add same fragment again (duplicate)
    EXPECT_FALSE(reassembler->addFragment(fragment));

    // Verify statistics
    auto stats = reassembler->getStatistics();
    EXPECT_EQ(stats.total_fragments, 2);  // Both attempts counted
    EXPECT_EQ(stats.duplicate_fragments, 1);
    EXPECT_EQ(stats.complete_sequences, 0);  // Not complete yet
}

/**
 * @brief Test invalid fragment rejection
 */
TEST_F(PFTReassemblerTest, InvalidFragmentRejection) {
    auto stats_before = reassembler->getStatistics();

    // Test 1: Invalid sync bytes
    {
        PFTPacket invalid_sync;
        invalid_sync.sync_pattern = 0x0000;  // Invalid
        invalid_sync.pseq = 500;
        invalid_sync.findex = 0;
        invalid_sync.fcount = 1;

        EXPECT_FALSE(reassembler->addFragment(invalid_sync));
    }

    // Test 2: Fragment index >= fragment count
    {
        PFTPacket invalid_index;
        invalid_index.sync_pattern = constants::PFT_SYNC_PATTERN_1;
        invalid_index.pseq = 501;
        invalid_index.findex = 5;
        invalid_index.fcount = 5;  // Index should be < count

        EXPECT_FALSE(reassembler->addFragment(invalid_index));
    }

    // Test 3: Zero fragment count
    {
        PFTPacket zero_count;
        zero_count.sync_pattern = constants::PFT_SYNC_PATTERN_1;
        zero_count.pseq = 502;
        zero_count.findex = 0;
        zero_count.fcount = 0;  // Invalid

        EXPECT_FALSE(reassembler->addFragment(zero_count));
    }

    // Verify statistics
    auto stats_after = reassembler->getStatistics();
    EXPECT_GT(stats_after.invalid_fragments, stats_before.invalid_fragments);
    EXPECT_EQ(stats_after.complete_sequences, 0);
}

// ============================================================================
// Advanced Functionality Tests
// ============================================================================

/**
 * @brief Test missing fragment detection
 */
TEST_F(PFTReassemblerTest, MissingFragmentDetection) {
    const uint16_t pseq = 600;
    const uint16_t fcount = 5;

    auto fragments = createCompleteSequence(pseq, fcount);

    // Add all except fragment 2
    ASSERT_TRUE(reassembler->addFragment(fragments[0]));
    ASSERT_TRUE(reassembler->addFragment(fragments[1]));
    // Skip fragments[2]
    ASSERT_TRUE(reassembler->addFragment(fragments[3]));
    ASSERT_TRUE(reassembler->addFragment(fragments[4]));

    // Should not be complete
    EXPECT_FALSE(reassembler->isComplete(pseq));

    // Check fragment count
    EXPECT_EQ(reassembler->getFragmentCount(pseq), 4);

    // Try to get packet (should fail)
    auto reassembled = reassembler->getReassembledPacket(pseq);
    EXPECT_FALSE(reassembled.has_value());
}

/**
 * @brief Test timeout-based cleanup
 */
TEST_F(PFTReassemblerTest, TimeoutCleanup) {
    const uint16_t pseq = 700;

    // Add incomplete sequence
    auto fragment = createFragment(pseq, 0, 3);
    ASSERT_TRUE(reassembler->addFragment(fragment));

    // Verify sequence is active
    auto active = reassembler->getActiveSequences();
    EXPECT_EQ(active.size(), 1);

    // Wait for timeout (use shorter timeout for test)
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    // Clear old sequences with very short timeout
    reassembler->clearOldSequences(std::chrono::seconds(0));

    // Verify sequence was cleaned up
    active = reassembler->getActiveSequences();
    EXPECT_EQ(active.size(), 0);

    // Verify statistics
    auto stats = reassembler->getStatistics();
    EXPECT_GT(stats.lost_sequences, 0);
}

/**
 * @brief Test multiple concurrent sequences
 */
TEST_F(PFTReassemblerTest, MultipleConcurrentSequences) {
    const uint16_t pseq1 = 800;
    const uint16_t pseq2 = 801;
    const uint16_t pseq3 = 802;

    // Create fragments for multiple sequences
    auto seq1_fragments = createCompleteSequence(pseq1, 3);
    auto seq2_fragments = createCompleteSequence(pseq2, 4);
    auto seq3_fragments = createCompleteSequence(pseq3, 2);

    // Add fragments from all sequences in interleaved order
    ASSERT_TRUE(reassembler->addFragment(seq1_fragments[0]));
    ASSERT_TRUE(reassembler->addFragment(seq2_fragments[0]));
    ASSERT_TRUE(reassembler->addFragment(seq1_fragments[1]));
    ASSERT_TRUE(reassembler->addFragment(seq3_fragments[0]));
    ASSERT_TRUE(reassembler->addFragment(seq2_fragments[1]));
    ASSERT_TRUE(reassembler->addFragment(seq1_fragments[2]));
    ASSERT_TRUE(reassembler->addFragment(seq3_fragments[1]));
    ASSERT_TRUE(reassembler->addFragment(seq2_fragments[2]));
    ASSERT_TRUE(reassembler->addFragment(seq2_fragments[3]));

    // All sequences should be complete
    EXPECT_TRUE(reassembler->isComplete(pseq1));
    EXPECT_TRUE(reassembler->isComplete(pseq2));
    EXPECT_TRUE(reassembler->isComplete(pseq3));

    // Verify active sequences
    auto active = reassembler->getActiveSequences();
    EXPECT_EQ(active.size(), 3);

    // Retrieve all sequences
    auto packet1 = reassembler->getReassembledPacket(pseq1);
    auto packet2 = reassembler->getReassembledPacket(pseq2);
    auto packet3 = reassembler->getReassembledPacket(pseq3);

    ASSERT_TRUE(packet1.has_value());
    ASSERT_TRUE(packet2.has_value());
    ASSERT_TRUE(packet3.has_value());

    // Verify statistics
    auto stats = reassembler->getStatistics();
    EXPECT_EQ(stats.complete_sequences, 3);
    EXPECT_EQ(stats.total_fragments, 9);
}

/**
 * @brief Test FEC recovery attempt
 */
TEST_F(PFTReassemblerTest, FECRecoveryAttempt) {
    const uint16_t pseq = 900;

    // Create fragment with FEC enabled
    auto fragment = createFragment(pseq, 0, 3);
    fragment.fec_type = 1;  // Enable FEC

    ASSERT_TRUE(reassembler->addFragment(fragment));

    // Try FEC recovery (should detect missing fragments)
    bool recovery_result = reassembler->performFECRecovery(pseq);

    // Current implementation doesn't have full Reed-Solomon,
    // so recovery should fail for incomplete sequences
    EXPECT_FALSE(recovery_result);

    // But if sequence is complete, recovery should succeed
    auto complete_fragments = createCompleteSequence(1000, 1);
    complete_fragments[0].fec_type = 1;
    ASSERT_TRUE(reassembler->addFragment(complete_fragments[0]));

    recovery_result = reassembler->performFECRecovery(1000);
    EXPECT_TRUE(recovery_result);  // Already complete
}

// ============================================================================
// Statistics and Configuration Tests
// ============================================================================

/**
 * @brief Test statistics tracking
 */
TEST_F(PFTReassemblerTest, StatisticsTracking) {
    auto stats = reassembler->getStatistics();

    // Initial state
    EXPECT_EQ(stats.total_fragments, 0);
    EXPECT_EQ(stats.complete_sequences, 0);
    EXPECT_EQ(stats.lost_sequences, 0);

    // Add complete sequence
    auto fragments = createCompleteSequence(1100, 3);
    for (const auto& fragment : fragments) {
        reassembler->addFragment(fragment);
    }

    stats = reassembler->getStatistics();
    EXPECT_EQ(stats.total_fragments, 3);
    EXPECT_EQ(stats.complete_sequences, 1);

    // Calculate rates
    EXPECT_GE(stats.get_success_rate(), 0.0);
    EXPECT_LE(stats.get_success_rate(), 100.0);

    // Reset statistics
    reassembler->resetStatistics();
    stats = reassembler->getStatistics();

    EXPECT_EQ(stats.total_fragments, 0);
    EXPECT_EQ(stats.complete_sequences, 0);
}

/**
 * @brief Test configuration management
 */
TEST_F(PFTReassemblerTest, ConfigurationManagement) {
    // Get default config
    auto config = reassembler->getConfig();
    EXPECT_GT(config.max_sequence_age.count(), 0);
    EXPECT_GT(config.max_active_sequences, 0);

    // Update config
    PFTReassembler::Config new_config;
    new_config.max_sequence_age = std::chrono::seconds(10);
    new_config.max_active_sequences = 512;
    new_config.enable_fec_recovery = false;
    new_config.strict_ordering = true;

    reassembler->setConfig(new_config);

    // Verify updated config
    config = reassembler->getConfig();
    EXPECT_EQ(config.max_sequence_age, std::chrono::seconds(10));
    EXPECT_EQ(config.max_active_sequences, 512);
    EXPECT_FALSE(config.enable_fec_recovery);
    EXPECT_TRUE(config.strict_ordering);
}

/**
 * @brief Test strict ordering mode
 */
TEST_F(PFTReassemblerTest, StrictOrderingMode) {
    // Enable strict ordering
    PFTReassembler::Config config;
    config.strict_ordering = true;
    reassembler->setConfig(config);

    const uint16_t pseq = 1200;
    auto fragments = createCompleteSequence(pseq, 3);

    // Add fragment 0 (should succeed)
    EXPECT_TRUE(reassembler->addFragment(fragments[0]));

    // Try to add fragment 2 before fragment 1 (should fail in strict mode)
    EXPECT_FALSE(reassembler->addFragment(fragments[2]));

    // Add fragment 1 (should succeed)
    EXPECT_TRUE(reassembler->addFragment(fragments[1]));

    // Now add fragment 2 (should succeed)
    EXPECT_TRUE(reassembler->addFragment(fragments[2]));
}

// ============================================================================
// Edge Cases
// ============================================================================

/**
 * @brief Test sequence number wraparound
 */
TEST_F(PFTReassemblerTest, SequenceNumberWraparound) {
    // Test with sequence numbers near uint16_t limit
    const uint16_t pseq1 = 65534;
    const uint16_t pseq2 = 65535;
    const uint16_t pseq3 = 0;  // Wraparound

    auto frag1 = createFragment(pseq1, 0, 1);
    auto frag2 = createFragment(pseq2, 0, 1);
    auto frag3 = createFragment(pseq3, 0, 1);

    EXPECT_TRUE(reassembler->addFragment(frag1));
    EXPECT_TRUE(reassembler->addFragment(frag2));
    EXPECT_TRUE(reassembler->addFragment(frag3));

    EXPECT_TRUE(reassembler->isComplete(pseq1));
    EXPECT_TRUE(reassembler->isComplete(pseq2));
    EXPECT_TRUE(reassembler->isComplete(pseq3));
}

/**
 * @brief Test large fragment count
 */
TEST_F(PFTReassemblerTest, LargeFragmentCount) {
    const uint16_t pseq = 1300;
    const uint16_t fcount = 100;  // Large number of fragments

    auto fragments = createCompleteSequence(pseq, fcount, 50);

    // Add all fragments
    for (const auto& fragment : fragments) {
        ASSERT_TRUE(reassembler->addFragment(fragment));
    }

    EXPECT_TRUE(reassembler->isComplete(pseq));

    auto reassembled = reassembler->getReassembledPacket(pseq);
    ASSERT_TRUE(reassembled.has_value());
    EXPECT_EQ(reassembled->size(), fcount * 50);
}

/**
 * @brief Test empty payload fragments
 */
TEST_F(PFTReassemblerTest, EmptyPayloadFragments) {
    const uint16_t pseq = 1400;

    // Create fragments with empty payloads
    auto fragments = createCompleteSequence(pseq, 3, 0);  // 0-byte payloads

    for (const auto& fragment : fragments) {
        EXPECT_TRUE(reassembler->addFragment(fragment));
    }

    EXPECT_TRUE(reassembler->isComplete(pseq));

    auto reassembled = reassembler->getReassembledPacket(pseq);
    ASSERT_TRUE(reassembled.has_value());
    EXPECT_EQ(reassembled->size(), 0);  // Total size should be 0
}

// ============================================================================
// Main
// ============================================================================

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
