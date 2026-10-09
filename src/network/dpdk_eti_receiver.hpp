/**
 * @file dpdk_eti_receiver.hpp
 * @brief DPDK-based Ultra-Low Latency ETI Stream Receiver
 * 
 * Kernel bypass networking implementation using DPDK for <0.1μs latency
 * ETI-over-IP reception with zero-copy packet processing.
 * 
 * Performance Targets:
 * - Network Latency: <0.1μs (kernel bypass)
 * - Packet Processing: 100M+ packets/second
 * - Memory Efficiency: Zero-copy packet handling
 * - CPU Efficiency: Poll-mode drivers
 * 
 * @author Performance Optimization Agent
 * @date 2025
 */

#pragma once

#include <QObject>
#include <memory>
#include <atomic>
#include <chrono>
#include <functional>
#include <span>

// DPDK headers
#ifdef DPDK_ENABLED
#include <rte_config.h>
#include <rte_eal.h>
#include <rte_ethdev.h>
#include <rte_mbuf.h>
#include <rte_mempool.h>
#include <rte_ring.h>
#include <rte_lcore.h>
#include <rte_launch.h>
#include <rte_atomic.h>
#include <rte_cycles.h>
#include <rte_prefetch.h>
#include <rte_branch_prediction.h>
#endif

#include "../core/ultra_performance_eti_processor.hpp"

namespace eti::network {

/**
 * @brief DPDK configuration for ultra-performance
 */
struct dpdk_config {
    uint16_t port_id{0};
    uint16_t queue_id{0};
    uint32_t nb_rxd{1024};      // RX descriptors
    uint32_t nb_txd{1024};      // TX descriptors
    uint32_t mempool_size{8192}; // Memory pool size
    uint32_t cache_size{256};    // Per-core cache
    uint16_t mtu{9000};          // Jumbo frames for efficiency
    
    // Performance tuning
    bool enable_rss{true};       // Receive Side Scaling
    bool enable_lro{true};       // Large Receive Offload
    bool enable_tso{true};       // TCP Segmentation Offload
    bool enable_hw_checksum{true}; // Hardware checksum offload
    
    // ETI-specific configuration
    uint16_t eti_udp_port{9200};     // Standard ETI-over-IP port
    uint32_t eti_multicast_group{0xEFC00001}; // 239.192.0.1
    uint32_t max_concurrent_streams{16}; // Maximum streams
};

/**
 * @brief Ultra-performance network statistics
 */
struct network_performance_stats {
    std::atomic<uint64_t> packets_received{0};
    std::atomic<uint64_t> packets_dropped{0};
    std::atomic<uint64_t> bytes_received{0};
    std::atomic<uint64_t> eti_frames_extracted{0};
    std::atomic<uint64_t> processing_cycles{0};
    std::atomic<double> packets_per_second{0.0};
    std::atomic<double> bytes_per_second{0.0};
    std::atomic<double> average_latency_ns{0.0};
    std::atomic<double> cpu_utilization{0.0};
    
    // Performance metrics
    std::atomic<bool> meets_latency_target{false};
    std::atomic<bool> meets_throughput_target{false};
    std::atomic<double> network_efficiency{0.0};
};

#ifdef DPDK_ENABLED
/**
 * @brief DPDK-based ETI stream receiver with kernel bypass
 */
class DpdkEtiReceiver : public QObject {
    Q_OBJECT

private:
    // DPDK resources
    struct rte_mempool* mbuf_pool_{nullptr};
    struct rte_ring* processing_ring_{nullptr};
    uint16_t port_id_{0};
    uint16_t queue_id_{0};
    
    // Configuration
    dpdk_config config_;
    
    // Performance tracking
    network_performance_stats stats_;
    std::chrono::high_resolution_clock::time_point start_time_;
    
    // Processing components
    std::unique_ptr<eti::ultra_performance::UltraPerformanceETIProcessor> eti_processor_;
    
    // Control
    std::atomic<bool> running_{false};
    std::atomic<bool> initialized_{false};
    
    // Receive burst configuration
    static constexpr uint16_t RX_BURST_SIZE = 32;
    static constexpr uint16_t RING_SIZE = 2048;
    
public:
    explicit DpdkEtiReceiver(QObject* parent = nullptr);
    ~DpdkEtiReceiver();
    
    /**
     * @brief Initialize DPDK and configure network interface
     * @param config DPDK configuration
     * @return true if initialization successful
     */
    bool initialize(const dpdk_config& config);
    
    /**
     * @brief Start receiving ETI streams
     * @return true if started successfully
     */
    bool start_receiving();
    
    /**
     * @brief Stop receiving
     */
    void stop_receiving();
    
    /**
     * @brief Get current network performance statistics
     */
    network_performance_stats get_statistics() const;
    
    /**
     * @brief Get DPDK port statistics
     */
    struct rte_eth_stats get_port_stats() const;
    
    /**
     * @brief Check if performance targets are met
     * @return true if all performance targets achieved
     */
    bool meets_performance_targets() const;
    
    /**
     * @brief Calculate network performance score (0-10)
     */
    double calculate_network_performance_score() const;
    
    /**
     * @brief Enable/disable performance monitoring
     */
    void set_performance_monitoring(bool enabled);

signals:
    /**
     * @brief Emitted when ETI frame is received and processed
     */
    void eti_frame_received(const std::span<const uint8_t>& frame_data, 
                           std::chrono::nanoseconds network_latency);
    
    /**
     * @brief Emitted when performance targets are achieved
     */
    void ultra_performance_achieved(double network_score, double processing_score);
    
    /**
     * @brief Emitted on network statistics update
     */
    void network_stats_updated(const network_performance_stats& stats);

private:
    /**
     * @brief Initialize DPDK environment
     */
    bool init_dpdk_environment();
    
    /**
     * @brief Configure network port
     */
    bool configure_network_port();
    
    /**
     * @brief Setup memory pools
     */
    bool setup_memory_pools();
    
    /**
     * @brief Main packet processing loop (runs on dedicated core)
     */
    int packet_processing_loop();
    
    /**
     * @brief Process received packets and extract ETI frames
     */
    void process_packet_burst(struct rte_mbuf** packets, uint16_t nb_packets);
    
    /**
     * @brief Extract ETI frame from UDP packet
     */
    bool extract_eti_frame(struct rte_mbuf* packet, 
                          std::span<uint8_t>& frame_data);
    
    /**
     * @brief Validate ETI-over-IP packet format
     */
    bool validate_eti_packet(struct rte_mbuf* packet);
    
    /**
     * @brief Update performance statistics
     */
    void update_performance_stats();
    
    /**
     * @brief Calculate packet processing latency
     */
    std::chrono::nanoseconds calculate_packet_latency(struct rte_mbuf* packet);
    
    // DPDK callback functions
    static int launch_packet_processing(void* arg);
    static void signal_handler(int signum);
    
    // Performance optimization functions
    inline void prefetch_packet_data(struct rte_mbuf* packet) {
        rte_prefetch0(rte_pktmbuf_mtod(packet, void*));
    }
    
    inline uint64_t get_timestamp_cycles() {
        return rte_rdtsc();
    }
    
    inline double cycles_to_nanoseconds(uint64_t cycles) {
        return static_cast<double>(cycles) / (rte_get_tsc_hz() / 1e9);
    }
};

#else
/**
 * @brief Fallback implementation when DPDK is not available
 */
class DpdkEtiReceiver : public QObject {
    Q_OBJECT

public:
    explicit DpdkEtiReceiver(QObject* parent = nullptr) : QObject(parent) {
        qWarning() << "DPDK not available - falling back to standard networking";
    }
    
    bool initialize(const dpdk_config& config) {
        Q_UNUSED(config)
        return false;
    }
    
    bool start_receiving() { return false; }
    void stop_receiving() {}
    
    network_performance_stats get_statistics() const {
        return network_performance_stats{};
    }
    
    bool meets_performance_targets() const { return false; }
    double calculate_network_performance_score() const { return 0.0; }
    void set_performance_monitoring(bool enabled) { Q_UNUSED(enabled) }

signals:
    void eti_frame_received(const std::span<const uint8_t>& frame_data, 
                           std::chrono::nanoseconds network_latency);
    void ultra_performance_achieved(double network_score, double processing_score);
    void network_stats_updated(const network_performance_stats& stats);
};
#endif

/**
 * @brief Factory function for creating DPDK receiver
 */
std::unique_ptr<DpdkEtiReceiver> create_dpdk_receiver(const dpdk_config& config = {});

/**
 * @brief Check if DPDK is available and properly configured
 */
bool is_dpdk_available();

/**
 * @brief Get optimal DPDK configuration for the system
 */
dpdk_config get_optimal_dpdk_config();

/**
 * @brief Performance benchmark for DPDK vs standard networking
 */
struct networking_benchmark_result {
    double dpdk_latency_ns;
    double standard_latency_ns;
    double dpdk_throughput_pps;
    double standard_throughput_pps;
    double latency_improvement_percent;
    double throughput_improvement_percent;
    bool dpdk_meets_targets;
};

/**
 * @brief Run networking performance benchmark
 */
networking_benchmark_result benchmark_networking_performance(
    const dpdk_config& config, 
    std::chrono::seconds duration = std::chrono::seconds{10});

} // namespace eti::network