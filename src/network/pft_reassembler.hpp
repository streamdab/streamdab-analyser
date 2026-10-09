/**
 * @file pft_reassembler.hpp
 * @brief PFT (Protection/Fragmentation/Transport) Packet Reassembler
 *
 * Implements ETSI TS 102 693 compliant PFT packet reassembly for EDI over IP.
 *
 * Features:
 * - Fragment collection and ordering
 * - Sequence-based reassembly
 * - Out-of-order fragment handling
 * - Timeout-based cleanup
 * - Forward Error Correction (FEC) recovery
 * - Comprehensive statistics tracking
 *
 * Reference: ETSI TS 102 693 V1.1.2 (2011-01) Section 7
 *
 * @author Backend Developer
 * @date 2025-10-19
 * @copyright Copyright (c) 2025 StreamDAB Analyser Team
 */

#ifndef PFT_REASSEMBLER_HPP
#define PFT_REASSEMBLER_HPP

#include "edi_types.hpp"
#include <map>
#include <vector>
#include <optional>
#include <chrono>
#include <mutex>
#include <memory>

namespace edi {

/**
 * @brief PFT packet reassembly engine
 *
 * Reassembles fragmented PFT packets into complete AF packets
 * following ETSI TS 102 693 Section 7 specification.
 *
 * Thread-safe implementation for concurrent network reception.
 */
class PFTReassembler {
public:
    /**
     * @brief Reassembly statistics structure
     */
    struct Stats {
        uint64_t total_fragments{0};        // Total fragments received
        uint64_t complete_sequences{0};     // Successfully reassembled sequences
        uint64_t fec_recoveries{0};         // Successful FEC recoveries
        uint64_t lost_sequences{0};         // Sequences lost (timeout/incomplete)
        uint64_t duplicate_fragments{0};    // Duplicate fragments received
        uint64_t invalid_fragments{0};      // Invalid/malformed fragments
        uint64_t out_of_order_fragments{0}; // Fragments received out of order
        uint64_t active_sequences{0};       // Currently active sequences
        std::chrono::steady_clock::time_point last_reset;

        Stats() : last_reset(std::chrono::steady_clock::now()) {}

        /**
         * @brief Calculate fragment loss rate
         * @return Loss rate as percentage (0.0-100.0)
         */
        double get_loss_rate() const;

        /**
         * @brief Calculate FEC recovery effectiveness
         * @return FEC recovery rate as percentage (0.0-100.0)
         */
        double get_fec_recovery_rate() const;

        /**
         * @brief Get reassembly success rate
         * @return Success rate as percentage (0.0-100.0)
         */
        double get_success_rate() const;

        /**
         * @brief Reset all statistics
         */
        void reset();
    };

    /**
     * @brief Configuration for reassembler behavior
     */
    struct Config {
        std::chrono::seconds max_sequence_age{5};   // Maximum sequence age before cleanup
        size_t max_active_sequences{256};           // Maximum concurrent sequences
        bool enable_fec_recovery{true};             // Enable FEC recovery attempts
        bool strict_ordering{false};                // Require strict fragment ordering
        size_t max_fragment_size{1500};             // Maximum expected fragment size
    };

    /**
     * @brief Construct reassembler with default configuration
     */
    PFTReassembler();

    /**
     * @brief Construct reassembler with custom configuration
     * @param config Reassembler configuration
     */
    explicit PFTReassembler(const Config& config);

    /**
     * @brief Destructor
     */
    ~PFTReassembler();

    // Disable copy (contains mutex)
    PFTReassembler(const PFTReassembler&) = delete;
    PFTReassembler& operator=(const PFTReassembler&) = delete;

    // Disable move (contains mutex, so a defaulted move would be deleted anyway)
    PFTReassembler(PFTReassembler&&) = delete;
    PFTReassembler& operator=(PFTReassembler&&) = delete;

    /**
     * @brief Add a PFT fragment for reassembly
     *
     * Adds a fragment to the appropriate sequence. If the sequence
     * becomes complete, it can be retrieved with getReassembledPacket().
     *
     * @param fragment PFT fragment packet
     * @return true if fragment was accepted, false if invalid/duplicate
     */
    bool addFragment(const PFTPacket& fragment);

    /**
     * @brief Check if a sequence is complete
     *
     * A sequence is complete when all fragments have been received
     * and successfully reassembled.
     *
     * @param pseq Protocol sequence number
     * @return true if sequence is complete and ready for retrieval
     */
    bool isComplete(uint16_t pseq) const;

    /**
     * @brief Get reassembled packet for a complete sequence
     *
     * Retrieves and removes the reassembled packet from internal storage.
     * Should only be called after isComplete() returns true.
     *
     * @param pseq Protocol sequence number
     * @return Reassembled packet data, or std::nullopt if not available
     */
    std::optional<std::vector<uint8_t>> getReassembledPacket(uint16_t pseq);

    /**
     * @brief Attempt FEC recovery for incomplete sequence
     *
     * Attempts to recover missing fragments using Forward Error Correction
     * if FEC parameters were provided in the PFT packets.
     *
     * @param pseq Protocol sequence number
     * @return true if recovery successful, false otherwise
     */
    bool performFECRecovery(uint16_t pseq);

    /**
     * @brief Clear old/incomplete sequences
     *
     * Removes sequences that have exceeded the maximum age without
     * completing, freeing up resources and preventing memory leaks.
     *
     * @param max_age Maximum sequence age before cleanup (default: 5 seconds)
     */
    void clearOldSequences(std::chrono::seconds max_age = std::chrono::seconds(5));

    /**
     * @brief Get current reassembly statistics
     * @return Current statistics snapshot
     */
    Stats getStatistics() const;

    /**
     * @brief Reset statistics counters
     */
    void resetStatistics();

    /**
     * @brief Get list of active sequence numbers
     * @return Vector of currently active sequence numbers
     */
    std::vector<uint16_t> getActiveSequences() const;

    /**
     * @brief Get fragment count for a specific sequence
     * @param pseq Protocol sequence number
     * @return Current fragment count, or 0 if sequence not found
     */
    size_t getFragmentCount(uint16_t pseq) const;

    /**
     * @brief Update reassembler configuration
     * @param config New configuration
     */
    void setConfig(const Config& config);

    /**
     * @brief Get current configuration
     * @return Current configuration
     */
    Config getConfig() const;

private:
    /**
     * @brief Internal sequence state tracking
     */
    struct SequenceState {
        uint16_t expected_count{0};                          // Total expected fragments
        std::map<uint16_t, std::vector<uint8_t>> fragments;  // findex -> payload
        std::chrono::steady_clock::time_point last_update;   // Last fragment received
        bool complete{false};                                // All fragments received
        bool fec_available{false};                           // FEC data available
        uint16_t rsk{0};                                     // Reed-Solomon K parameter
        uint16_t rsz{0};                                     // Reed-Solomon Z parameter

        SequenceState() : last_update(std::chrono::steady_clock::now()) {}

        /**
         * @brief Check if all fragments received
         * @return true if fragment count matches expected
         */
        bool is_complete() const {
            return fragments.size() == expected_count && expected_count > 0;
        }

        /**
         * @brief Get missing fragment indices
         * @return Vector of missing fragment indices
         */
        std::vector<uint16_t> get_missing_fragments() const;

        /**
         * @brief Calculate total payload size
         * @return Sum of all fragment payload sizes
         */
        size_t get_total_size() const;
    };

    /**
     * @brief Validate fragment before adding to sequence
     * @param fragment PFT fragment to validate
     * @return true if fragment is valid
     */
    bool validateFragment(const PFTPacket& fragment) const;

    /**
     * @brief Reassemble fragments into complete packet
     * @param state Sequence state with all fragments
     * @return Reassembled packet data
     */
    std::vector<uint8_t> reassembleFragments(const SequenceState& state) const;

    /**
     * @brief Attempt Reed-Solomon FEC recovery
     * @param state Sequence state with partial fragments
     * @return true if recovery successful
     */
    bool attemptFECRecovery(SequenceState& state);

    /**
     * @brief Clean up single sequence
     * @param pseq Sequence number to clean up
     * @param reason Cleanup reason for statistics
     */
    void cleanupSequence(uint16_t pseq, const std::string& reason);

    // Internal state
    mutable std::mutex mutex_;                           // Thread safety
    std::map<uint16_t, SequenceState> sequences_;       // pseq -> state
    Stats stats_;                                        // Statistics
    Config config_;                                      // Configuration

    // Constants
    static constexpr size_t DEFAULT_MAX_SEQUENCES = 256;
    static constexpr std::chrono::seconds DEFAULT_MAX_AGE{5};
};

} // namespace edi

#endif // PFT_REASSEMBLER_HPP
