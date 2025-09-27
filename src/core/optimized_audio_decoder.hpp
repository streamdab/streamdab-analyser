/**
 * @file optimized_audio_decoder.hpp
 * @brief High-Performance Audio Decoder with <15ms Latency
 * 
 * Advanced audio decoding engine optimized for broadcast industry standards
 * with SIMD-accelerated processing, lock-free ring buffers, and multi-codec
 * support. Target: <15ms latency with >1000 fps throughput.
 * 
 * @author Agent 20 - Performance Optimization Specialist
 * @date 2025-09-26
 */

#pragma once

#include <QObject>
#include <QByteArray>
#include <QAudioOutput>
#include <QAudioFormat>
#include <immintrin.h>
#include <memory>
#include <atomic>
#include <array>
#include <thread>
#include <condition_variable>
#include <mutex>
#include <chrono>

// External audio codec libraries
extern "C" {
    #include <libavcodec/avcodec.h>
    #include <libavformat/avformat.h>
    #include <libswresample/swresample.h>
    #include <faad.h>  // AAC decoder
}

namespace eti::audio {

/**
 * @brief Supported audio codec types with optimization levels
 */
enum class OptimizedAudioCodec {
    MPEG1_Layer2_Optimized,    // DAB audio with SIMD optimization
    AAC_LC_Optimized,          // DAB+ AAC-LC with vectorized processing
    HE_AAC_v1_Optimized,       // HE-AAC v1 with specialized buffers
    HE_AAC_v2_Optimized,       // HE-AAC v2 with SBR optimization
    Unknown
};

/**
 * @brief Lock-free audio buffer for zero-copy processing
 */
template<size_t SampleCount = 4096>
class LockFreeAudioBuffer {
public:
    struct AudioFrame {
        alignas(32) float samples[SampleCount * 2]; // Stereo samples, 32-byte aligned
        std::atomic<bool> ready{false};
        std::chrono::high_resolution_clock::time_point timestamp;
        size_t actual_samples{0};
        uint32_t sample_rate{48000};
        uint8_t channels{2};
    };
    
    LockFreeAudioBuffer();
    
    AudioFrame* acquire_write_frame();
    void release_write_frame(AudioFrame* frame);
    AudioFrame* acquire_read_frame();
    void release_read_frame(AudioFrame* frame);
    
    double get_buffer_utilization() const;
    size_t get_pending_samples() const;
    
private:
    static constexpr size_t BUFFER_SIZE = 64;
    static constexpr size_t INDEX_MASK = BUFFER_SIZE - 1;
    
    alignas(64) std::array<AudioFrame, BUFFER_SIZE> buffer_;
    alignas(64) std::atomic<size_t> write_index_{0};
    alignas(64) std::atomic<size_t> read_index_{0};
};

/**
 * @brief SIMD-optimized audio processing utilities
 */
class SimdAudioProcessor {
public:
    SimdAudioProcessor();
    
    /**
     * @brief Convert int16 samples to float with AVX2
     * @param input Input int16 samples (must be 32-byte aligned)
     * @param output Output float samples (must be 32-byte aligned)  
     * @param sample_count Number of samples to convert
     */
    void convert_int16_to_float_avx2(const int16_t* __restrict__ input, 
                                    float* __restrict__ output, 
                                    size_t sample_count) const;
    
    /**
     * @brief Apply audio gain with SIMD optimization
     * @param samples Audio samples (32-byte aligned)
     * @param sample_count Number of samples
     * @param gain Gain factor (0.0 - 2.0)
     */
    void apply_gain_simd(float* __restrict__ samples, size_t sample_count, float gain) const;
    
    /**
     * @brief Mix stereo channels with vectorized processing
     * @param left_channel Left channel samples
     * @param right_channel Right channel samples
     * @param output Interleaved stereo output
     * @param sample_count Samples per channel
     */
    void mix_stereo_channels_avx2(const float* __restrict__ left_channel,
                                 const float* __restrict__ right_channel,
                                 float* __restrict__ output,
                                 size_t sample_count) const;
    
    /**
     * @brief Apply low-pass filter with SIMD
     * @param samples Audio samples
     * @param sample_count Number of samples
     * @param cutoff_freq Cutoff frequency (Hz)
     * @param sample_rate Sample rate (Hz)
     */
    void apply_lowpass_filter_simd(float* __restrict__ samples, 
                                  size_t sample_count,
                                  float cutoff_freq, 
                                  uint32_t sample_rate) const;
    
private:
    alignas(32) float filter_coefficients_[16];
    bool avx2_supported_;
    
    void initialize_filter_coefficients();
};

/**
 * @brief Memory pool for audio frame allocation
 */
class AudioFrameMemoryPool {
public:
    explicit AudioFrameMemoryPool(size_t pool_size = 512);
    ~AudioFrameMemoryPool();
    
    float* allocate_audio_buffer(size_t sample_count);
    void deallocate_audio_buffer(float* buffer);
    
    struct PoolStatistics {
        size_t total_blocks;
        size_t used_blocks;
        size_t peak_usage;
        double utilization_percent;
        size_t allocation_failures;
    };
    
    PoolStatistics get_statistics() const;
    void reset_statistics();
    
private:
    struct alignas(32) AudioBlock {
        float* data;
        size_t capacity;
        AudioBlock* next;
        bool in_use;
        std::chrono::high_resolution_clock::time_point allocation_time;
    };
    
    mutable std::mutex pool_mutex_;
    std::vector<std::unique_ptr<float[]>> memory_regions_;
    AudioBlock* free_list_;
    std::atomic<size_t> used_count_{0};
    std::atomic<size_t> peak_usage_{0};
    std::atomic<size_t> allocation_failures_{0};
    const size_t pool_size_;
};

/**
 * @brief High-performance optimized audio decoder
 */
class OptimizedAudioDecoder : public QObject {
    Q_OBJECT
    
public:
    explicit OptimizedAudioDecoder(QObject* parent = nullptr);
    ~OptimizedAudioDecoder();
    
    /**
     * @brief Initialize decoder with optimization settings
     * @param enable_real_time Enable real-time processing mode
     * @param target_latency_ms Target latency in milliseconds
     * @return true if initialization successful
     */
    bool initialize(bool enable_real_time = true, double target_latency_ms = 15.0);
    
    /**
     * @brief Decode audio frame with SIMD optimization
     * @param service_id Service identifier
     * @param encoded_data Encoded audio data
     * @param codec Codec type with optimization level
     * @return true if decoding successful
     * 
     * Performance Target: <15ms latency per frame
     */
    bool decode_audio_frame_optimized(uint32_t service_id,
                                     const QByteArray& encoded_data,
                                     OptimizedAudioCodec codec);
    
    /**
     * @brief Batch decode multiple frames for maximum throughput
     * @param frames Vector of encoded audio frames
     * @param codec Codec type
     * @return Number of successfully decoded frames
     * 
     * Performance Target: >1000 fps batch processing
     */
    size_t decode_batch_optimized(const std::vector<std::pair<uint32_t, QByteArray>>& frames,
                                 OptimizedAudioCodec codec);
    
    /**
     * @brief Enable real-time audio output
     * @param enabled Enable/disable real-time output
     */
    void set_real_time_output(bool enabled);
    
    /**
     * @brief Set audio output format
     * @param sample_rate Sample rate (Hz)
     * @param channels Number of channels
     * @param bit_depth Bit depth (16, 24, 32)
     */
    bool set_audio_format(uint32_t sample_rate, uint8_t channels, uint8_t bit_depth);
    
    /**
     * @brief Get decoded audio data
     * @param service_id Service identifier
     * @return Decoded audio samples or empty array if none available
     */
    QByteArray get_decoded_audio(uint32_t service_id);
    
    /**
     * @brief Performance metrics for audio decoding
     */
    struct AudioPerformanceMetrics {
        double decoding_fps;
        std::chrono::microseconds average_latency;
        std::chrono::microseconds peak_latency;
        double cpu_usage_percent;
        size_t memory_usage_bytes;
        size_t total_frames_decoded;
        size_t successful_decodes;
        double success_rate_percent;
        double buffer_utilization_percent;
        size_t buffer_underruns;
        size_t buffer_overruns;
    };
    
    AudioPerformanceMetrics get_performance_metrics() const;
    
    /**
     * @brief Reset performance counters
     */
    void reset_performance_counters();
    
    /**
     * @brief Configure decoder optimization level
     */
    enum class OptimizationLevel {
        QUALITY_FOCUSED,      // Best quality, higher latency
        BALANCED,            // Balance quality and performance
        PERFORMANCE_FOCUSED,  // Minimum latency, optimized throughput
        REAL_TIME_CRITICAL   // <15ms latency guaranteed
    };
    
    void set_optimization_level(OptimizationLevel level);
    
signals:
    /**
     * @brief Emitted when audio frame is decoded
     * @param service_id Service identifier
     * @param decoded_samples Number of decoded samples
     * @param decoding_time Time taken to decode (nanoseconds)
     */
    void audio_frame_decoded(uint32_t service_id, size_t decoded_samples, 
                           std::chrono::nanoseconds decoding_time);
    
    /**
     * @brief Emitted when performance target is achieved
     * @param current_latency Current decoding latency
     * @param target_latency Target latency
     */
    void performance_target_achieved(std::chrono::microseconds current_latency,
                                   std::chrono::microseconds target_latency);
    
    /**
     * @brief Emitted when buffer underrun occurs
     * @param service_id Service identifier
     * @param missed_samples Number of missed samples
     */
    void buffer_underrun(uint32_t service_id, size_t missed_samples);
    
    /**
     * @brief Emitted when decoding error occurs
     * @param service_id Service identifier
     * @param error_message Error description
     */
    void decoding_error(uint32_t service_id, const QString& error_message);

private slots:
    void handle_audio_output();
    void handle_performance_monitoring();
    void handle_buffer_management();

private:
    // Codec-specific decoders
    struct CodecContext {
        AVCodecContext* ffmpeg_context;
        faacDecHandle faad_handle;
        OptimizedAudioCodec codec_type;
        QAudioFormat audio_format;
        std::chrono::high_resolution_clock::time_point last_decode_time;
        std::atomic<uint64_t> frames_decoded{0};
        std::atomic<uint64_t> decode_errors{0};
    };
    
    // SIMD optimization components
    std::unique_ptr<SimdAudioProcessor> simd_processor_;
    
    // Memory management
    std::unique_ptr<AudioFrameMemoryPool> memory_pool_;
    std::unordered_map<uint32_t, std::unique_ptr<LockFreeAudioBuffer<>>> audio_buffers_;
    
    // Performance monitoring
    mutable std::mutex performance_mutex_;
    std::atomic<double> current_fps_{0.0};
    std::atomic<uint64_t> total_decoding_time_ns_{0};
    std::atomic<uint64_t> frames_processed_{0};
    std::atomic<uint64_t> successful_decodes_{0};
    
    // Real-time processing
    std::thread audio_output_thread_;
    std::thread performance_monitor_thread_;
    std::atomic<bool> real_time_active_{false};
    std::atomic<bool> processing_active_{false};
    
    // Codec contexts per service
    mutable std::mutex codec_mutex_;
    std::unordered_map<uint32_t, std::unique_ptr<CodecContext>> codec_contexts_;
    
    // Configuration
    OptimizationLevel optimization_level_;
    std::chrono::microseconds target_latency_;
    QAudioFormat output_format_;
    std::unique_ptr<QAudioOutput> audio_output_;
    
    // Performance targets
    static constexpr double TARGET_FPS_MINIMUM = 1000.0;
    static constexpr std::chrono::microseconds TARGET_LATENCY_MAX{15000}; // 15ms
    static constexpr size_t TARGET_MEMORY_MB = 30;
    
    // Core decoding methods
    bool decode_mpeg1_layer2_optimized(CodecContext* context, const QByteArray& data);
    bool decode_aac_optimized(CodecContext* context, const QByteArray& data);
    bool decode_he_aac_optimized(CodecContext* context, const QByteArray& data);
    
    // Context management
    CodecContext* get_or_create_codec_context(uint32_t service_id, OptimizedAudioCodec codec);
    void cleanup_codec_context(CodecContext* context);
    
    // Buffer management
    bool allocate_audio_buffers(uint32_t service_id);
    void manage_buffer_levels();
    
    // Performance optimization
    void optimize_decoder_settings();
    void update_performance_statistics();
    void monitor_system_resources();
    
    // SIMD detection and configuration
    bool detect_simd_capabilities();
    void configure_simd_optimizations();
    
    // Audio format conversion
    bool convert_to_output_format(const float* input_samples, size_t sample_count,
                                 QByteArray& output_data);
    
    // Quality monitoring
    void analyze_audio_quality(const float* samples, size_t sample_count);
    double calculate_thd_plus_noise(const float* samples, size_t sample_count, uint32_t sample_rate);
};

// Template implementations for LockFreeAudioBuffer
template<size_t SampleCount>
LockFreeAudioBuffer<SampleCount>::LockFreeAudioBuffer() {
    for (auto& frame : buffer_) {
        frame.ready.store(false, std::memory_order_relaxed);
        frame.actual_samples = 0;
    }
}

template<size_t SampleCount>
typename LockFreeAudioBuffer<SampleCount>::AudioFrame*
LockFreeAudioBuffer<SampleCount>::acquire_write_frame() {
    const size_t current_write = write_index_.load(std::memory_order_acquire);
    const size_t next_write = (current_write + 1) & INDEX_MASK;
    
    if (next_write == read_index_.load(std::memory_order_acquire)) {
        return nullptr; // Buffer full
    }
    
    AudioFrame* frame = &buffer_[current_write];
    if (frame->ready.load(std::memory_order_acquire)) {
        return nullptr; // Frame not yet consumed
    }
    
    if (write_index_.compare_exchange_weak(const_cast<size_t&>(current_write), next_write,
                                          std::memory_order_acq_rel)) {
        frame->timestamp = std::chrono::high_resolution_clock::now();
        return frame;
    }
    
    return nullptr;
}

template<size_t SampleCount>
void LockFreeAudioBuffer<SampleCount>::release_write_frame(AudioFrame* frame) {
    frame->ready.store(true, std::memory_order_release);
}

template<size_t SampleCount>
typename LockFreeAudioBuffer<SampleCount>::AudioFrame*
LockFreeAudioBuffer<SampleCount>::acquire_read_frame() {
    const size_t current_read = read_index_.load(std::memory_order_acquire);
    
    if (current_read == write_index_.load(std::memory_order_acquire)) {
        return nullptr; // Buffer empty
    }
    
    AudioFrame* frame = &buffer_[current_read];
    if (!frame->ready.load(std::memory_order_acquire)) {
        return nullptr; // Frame not ready
    }
    
    return frame;
}

template<size_t SampleCount>
void LockFreeAudioBuffer<SampleCount>::release_read_frame(AudioFrame* frame) {
    frame->ready.store(false, std::memory_order_release);
    
    const size_t current_read = read_index_.load(std::memory_order_acquire);
    const size_t next_read = (current_read + 1) & INDEX_MASK;
    read_index_.store(next_read, std::memory_order_release);
}

template<size_t SampleCount>
double LockFreeAudioBuffer<SampleCount>::get_buffer_utilization() const {
    const size_t write_pos = write_index_.load(std::memory_order_acquire);
    const size_t read_pos = read_index_.load(std::memory_order_acquire);
    
    size_t used_frames;
    if (write_pos >= read_pos) {
        used_frames = write_pos - read_pos;
    } else {
        used_frames = BUFFER_SIZE - read_pos + write_pos;
    }
    
    return (static_cast<double>(used_frames) / BUFFER_SIZE) * 100.0;
}

} // namespace eti::audio