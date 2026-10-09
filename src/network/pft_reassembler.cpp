/**
 * @file pft_reassembler.cpp
 * @brief PFT Packet Reassembler Implementation
 *
 * Implements ETSI TS 102 693 compliant PFT packet reassembly.
 *
 * @author Backend Developer
 * @date 2025-10-19
 * @copyright Copyright (c) 2025 StreamDAB Analyser Team
 */

#include "pft_reassembler.hpp"
#include <algorithm>
#include <numeric>
#include <sstream>

namespace edi {

// ============================================================================
// Statistics Implementation
// ============================================================================

double PFTReassembler::Stats::get_loss_rate() const {
    if (total_fragments == 0) {
        return 0.0;
    }
    return (static_cast<double>(lost_sequences) / static_cast<double>(total_fragments)) * 100.0;
}

double PFTReassembler::Stats::get_fec_recovery_rate() const {
    if (complete_sequences == 0) {
        return 0.0;
    }
    return (static_cast<double>(fec_recoveries) / static_cast<double>(complete_sequences)) * 100.0;
}

double PFTReassembler::Stats::get_success_rate() const {
    uint64_t total_sequences = complete_sequences + lost_sequences;
    if (total_sequences == 0) {
        return 0.0;
    }
    return (static_cast<double>(complete_sequences) / static_cast<double>(total_sequences)) * 100.0;
}

void PFTReassembler::Stats::reset() {
    total_fragments = 0;
    complete_sequences = 0;
    fec_recoveries = 0;
    lost_sequences = 0;
    duplicate_fragments = 0;
    invalid_fragments = 0;
    out_of_order_fragments = 0;
    active_sequences = 0;
    last_reset = std::chrono::steady_clock::now();
}

// ============================================================================
// SequenceState Implementation
// ============================================================================

std::vector<uint16_t> PFTReassembler::SequenceState::get_missing_fragments() const {
    std::vector<uint16_t> missing;

    for (uint16_t i = 0; i < expected_count; ++i) {
        if (fragments.find(i) == fragments.end()) {
            missing.push_back(i);
        }
    }

    return missing;
}

size_t PFTReassembler::SequenceState::get_total_size() const {
    return std::accumulate(
        fragments.begin(),
        fragments.end(),
        size_t(0),
        [](size_t sum, const auto& pair) {
            return sum + pair.second.size();
        }
    );
}

// ============================================================================
// PFTReassembler Implementation
// ============================================================================

PFTReassembler::PFTReassembler()
    : config_{}
{
    config_.max_sequence_age = DEFAULT_MAX_AGE;
    config_.max_active_sequences = DEFAULT_MAX_SEQUENCES;
    config_.enable_fec_recovery = true;
    config_.strict_ordering = false;
    config_.max_fragment_size = 1500;
}

PFTReassembler::PFTReassembler(const Config& config)
    : config_(config)
{
}

PFTReassembler::~PFTReassembler() = default;

bool PFTReassembler::addFragment(const PFTPacket& fragment) {
    std::lock_guard<std::mutex> lock(mutex_);

    // Validate fragment
    if (!validateFragment(fragment)) {
        ++stats_.invalid_fragments;
        return false;
    }

    ++stats_.total_fragments;

    const uint16_t pseq = static_cast<uint16_t>(fragment.pseq);
    const uint16_t findex = static_cast<uint16_t>(fragment.findex);
    const uint16_t fcount = static_cast<uint16_t>(fragment.fcount);

    // Get or create sequence state
    auto& state = sequences_[pseq];

    // Initialize sequence on first fragment
    if (state.expected_count == 0) {
        state.expected_count = fcount;
        state.last_update = std::chrono::steady_clock::now();

        // Store FEC parameters if available
        if (fragment.fec_type != 0) {
            state.fec_available = true;
            // FEC parameters would be extracted from fec_type
            // For now, we just mark FEC as available
        }
    } else {
        // Verify fragment count matches
        if (state.expected_count != fcount) {
            ++stats_.invalid_fragments;
            return false;
        }
    }

    // Check for duplicate
    if (state.fragments.find(findex) != state.fragments.end()) {
        ++stats_.duplicate_fragments;
        return false;  // Duplicate fragment, ignore
    }

    // Check for out-of-order reception
    if (!state.fragments.empty() && findex < state.fragments.rbegin()->first) {
        ++stats_.out_of_order_fragments;
    }

    // Add fragment to sequence
    state.fragments[findex] = fragment.payload;
    state.last_update = std::chrono::steady_clock::now();

    // Check if sequence is now complete
    if (state.fragments.size() == state.expected_count) {
        state.complete = true;
        ++stats_.complete_sequences;
    }

    // Update active sequences count
    stats_.active_sequences = sequences_.size();

    return true;
}

bool PFTReassembler::isComplete(uint16_t pseq) const {
    std::lock_guard<std::mutex> lock(mutex_);

    auto it = sequences_.find(pseq);
    if (it == sequences_.end()) {
        return false;
    }

    return it->second.is_complete();
}

std::optional<std::vector<uint8_t>> PFTReassembler::getReassembledPacket(uint16_t pseq) {
    std::lock_guard<std::mutex> lock(mutex_);

    auto it = sequences_.find(pseq);
    if (it == sequences_.end() || !it->second.is_complete()) {
        return std::nullopt;
    }

    // Reassemble fragments
    auto reassembled = reassembleFragments(it->second);

    // Remove sequence from active list
    sequences_.erase(it);
    stats_.active_sequences = sequences_.size();

    return reassembled;
}

bool PFTReassembler::performFECRecovery(uint16_t pseq) {
    std::lock_guard<std::mutex> lock(mutex_);

    if (!config_.enable_fec_recovery) {
        return false;
    }

    auto it = sequences_.find(pseq);
    if (it == sequences_.end()) {
        return false;
    }

    auto& state = it->second;

    // Check if FEC recovery is possible
    if (!state.fec_available) {
        return false;
    }

    // Check if sequence is already complete
    if (state.is_complete()) {
        return true;
    }

    // Attempt FEC recovery
    bool recovery_successful = attemptFECRecovery(state);

    if (recovery_successful) {
        ++stats_.fec_recoveries;
        state.complete = true;
        ++stats_.complete_sequences;
    }

    return recovery_successful;
}

void PFTReassembler::clearOldSequences(std::chrono::seconds max_age) {
    std::lock_guard<std::mutex> lock(mutex_);

    auto now = std::chrono::steady_clock::now();
    std::vector<uint16_t> to_remove;

    // Find sequences older than max_age
    for (const auto& [pseq, state] : sequences_) {
        auto age = std::chrono::duration_cast<std::chrono::seconds>(now - state.last_update);

        if (age >= max_age && !state.complete) {
            to_remove.push_back(pseq);
        }
    }

    // Remove old sequences
    for (uint16_t pseq : to_remove) {
        cleanupSequence(pseq, "timeout");
    }

    stats_.active_sequences = sequences_.size();
}

PFTReassembler::Stats PFTReassembler::getStatistics() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return stats_;
}

void PFTReassembler::resetStatistics() {
    std::lock_guard<std::mutex> lock(mutex_);
    stats_.reset();
}

std::vector<uint16_t> PFTReassembler::getActiveSequences() const {
    std::lock_guard<std::mutex> lock(mutex_);

    std::vector<uint16_t> active;
    active.reserve(sequences_.size());

    for (const auto& [pseq, _] : sequences_) {
        active.push_back(pseq);
    }

    return active;
}

size_t PFTReassembler::getFragmentCount(uint16_t pseq) const {
    std::lock_guard<std::mutex> lock(mutex_);

    auto it = sequences_.find(pseq);
    if (it == sequences_.end()) {
        return 0;
    }

    return it->second.fragments.size();
}

void PFTReassembler::setConfig(const Config& config) {
    std::lock_guard<std::mutex> lock(mutex_);
    config_ = config;
}

PFTReassembler::Config PFTReassembler::getConfig() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return config_;
}

// ============================================================================
// Private Methods
// ============================================================================

bool PFTReassembler::validateFragment(const PFTPacket& fragment) const {
    // Validate sync bytes using isValid() method
    if (!fragment.isValid()) {
        return false;
    }

    // Validate fragment indices
    if (fragment.findex >= fragment.fcount) {
        return false;
    }

    // Validate fragment count
    if (fragment.fcount == 0 || fragment.fcount > 1024) {  // Reasonable max fragments
        return false;
    }

    // Validate payload size
    if (fragment.payload.size() > config_.max_fragment_size) {
        return false;
    }

    // Validate strict ordering if required
    if (config_.strict_ordering) {
        auto it = sequences_.find(static_cast<uint16_t>(fragment.pseq));
        if (it != sequences_.end()) {
            // Check that fragment index is sequential
            if (!it->second.fragments.empty()) {
                uint16_t last_index = it->second.fragments.rbegin()->first;
                if (fragment.findex != last_index + 1) {
                    return false;
                }
            } else if (fragment.findex != 0) {
                return false;  // First fragment must be index 0
            }
        }
    }

    return true;
}

std::vector<uint8_t> PFTReassembler::reassembleFragments(const SequenceState& state) const {
    std::vector<uint8_t> reassembled;

    // Calculate total size
    size_t total_size = state.get_total_size();
    reassembled.reserve(total_size);

    // Concatenate fragments in order
    for (uint16_t i = 0; i < state.expected_count; ++i) {
        auto it = state.fragments.find(i);
        if (it != state.fragments.end()) {
            reassembled.insert(
                reassembled.end(),
                it->second.begin(),
                it->second.end()
            );
        }
    }

    return reassembled;
}

bool PFTReassembler::attemptFECRecovery(SequenceState& state) {
    // Basic FEC recovery implementation
    // Full Reed-Solomon implementation would go here

    // For now, just detect missing fragments
    auto missing = state.get_missing_fragments();

    if (missing.empty()) {
        return true;  // Already complete
    }

    // TODO: Implement Reed-Solomon FEC recovery
    // This would use the rsk and rsz parameters to recover missing fragments
    // For basic implementation, we just log the recovery attempt

    // Current implementation: cannot recover without full Reed-Solomon
    return false;
}

void PFTReassembler::cleanupSequence(uint16_t pseq, const std::string& reason) {
    auto it = sequences_.find(pseq);
    if (it != sequences_.end()) {
        if (!it->second.complete) {
            ++stats_.lost_sequences;
        }
        sequences_.erase(it);
    }
}

} // namespace edi
