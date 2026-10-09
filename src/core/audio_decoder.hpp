#pragma once

#include <QObject>
#include <QByteArray>
#include <QAudioFormat>
#include <QAudioOutput>
#include <QMutex>
#include <QTimer>
#include <memory>
#include <atomic>
#include <vector>

// Forward declarations for audio libraries
#ifdef USE_FAAD2
extern "C" {
#include <neaacdec.h>
}
#else
// Fallback declarations when FAAD2 is not available
struct NeAACDecStruct;
typedef NeAACDecStruct* NeAACDecHandle;
struct NeAACDecConfiguration;
typedef NeAACDecConfiguration* NeAACDecConfigurationPtr;
struct NeAACDecFrameInfo;
#endif

struct mpg123_handle_struct;
typedef mpg123_handle_struct mpg123_handle;

namespace eti::audio {

/**
 * @brief Professional Audio Decoder for DAB/DAB+ streams
 * 
 * Provides unified interface for DAB (MPEG-1 Layer 2) and DAB+ (AAC/HE-AAC)
 * decoding with real-time processing, quality monitoring, and audio level analysis.
 * 
 * Features:
 * - Multi-codec support (MPEG-1 Layer 2, AAC, HE-AAC v1/v2)
 * - Real-time audio level monitoring with peak/RMS calculation
 * - Signal quality assessment and error detection
 * - Professional audio format handling (16/24-bit PCM, float)
 * - Thread-safe operation with Qt signal/slot integration
 * - Memory pool optimization for zero-copy processing
 */
class audio_decoder : public QObject {
    Q_OBJECT
    
public:
    /**
     * @brief Audio codec types supported by the decoder
     */
    enum class codec_type {
        mpeg1_layer2,       ///< DAB MPEG-1 Layer 2 (libmpg123)
        aac_lc,            ///< AAC Low Complexity
        aac_plus,          ///< DAB+ HE-AAC v1 (FAAD2)
        aac_plus_v2,       ///< DAB+ HE-AAC v2 with PS (FAAD2)
        unknown            ///< Unknown or unsupported codec
    };
    
    /**
     * @brief Audio output format types
     */
    enum class output_format {
        pcm_16_le,         ///< 16-bit little-endian PCM
        pcm_24_le,         ///< 24-bit little-endian PCM
        pcm_32_float,      ///< 32-bit floating point PCM
        native             ///< Native decoder output format
    };
    
    /**
     * @brief Decoder configuration structure
     */
    struct decoder_config {
        output_format format = output_format::pcm_16_le;
        uint32_t sample_rate = 48000;        ///< Target sample rate (0 = auto)
        uint16_t channels = 2;               ///< Target channels (0 = auto)
        bool enable_error_concealment = true; ///< Enable audio error concealment
        bool enable_level_monitoring = true;  ///< Enable real-time level monitoring
        bool enable_quality_monitoring = true; ///< Enable signal quality analysis
        double gain_adjustment = 1.0;        ///< Audio gain (0.1-2.0)
        bool enable_downmix = false;         ///< Enable stereo to mono downmix
        uint32_t buffer_size_ms = 50;       ///< Internal buffer size in ms
    };
    
    /**
     * @brief Audio quality metrics structure
     */
    struct quality_metrics {
        double peak_level_db = -60.0;        ///< Peak level in dBFS
        double rms_level_db = -60.0;         ///< RMS level in dBFS
        double peak_level_linear = 0.0;      ///< Peak level (0.0-1.0)
        double rms_level_linear = 0.0;       ///< RMS level (0.0-1.0)
        double signal_quality = 100.0;      ///< Signal quality percentage
        double bit_error_rate = 0.0;         ///< Estimated bit error rate
        uint32_t frames_decoded = 0;         ///< Total frames decoded
        uint32_t frames_with_errors = 0;     ///< Frames with errors
        bool clipping_detected = false;      ///< Audio clipping detected
        bool silence_detected = false;       ///< Extended silence detected
    };
    
    /**
     * @brief Decoder status information
     */
    struct decoder_status {
        codec_type current_codec = codec_type::unknown;
        bool is_initialized = false;
        bool is_decoding = false;
        uint32_t input_sample_rate = 0;
        uint32_t output_sample_rate = 0;
        uint16_t input_channels = 0;
        uint16_t output_channels = 0;
        uint32_t bitrate_kbps = 0;
        QString codec_version;
        QString last_error;
        quality_metrics metrics;
    };

    explicit audio_decoder(QObject* parent = nullptr);
    ~audio_decoder();

    // Core functionality
    bool initialize(codec_type codec, const decoder_config& config);
    bool initialize(codec_type codec); // Uses default config
    bool decode_frame(const QByteArray& encoded_data, QByteArray& decoded_audio);
    bool decode_frame_to_float(const QByteArray& encoded_data, std::vector<float>& decoded_audio);
    void reset_decoder();
    void flush_buffers();

    // Configuration management
    bool set_config(const decoder_config& config);
    decoder_config get_config() const;
    bool reconfigure(const decoder_config& config);

    // Audio format management
    QAudioFormat get_qt_audio_format() const;
    output_format get_output_format() const;
    uint32_t get_output_sample_rate() const;
    uint16_t get_output_channels() const;
    uint16_t get_output_bit_depth() const;

    // Status and information
    decoder_status get_status() const;
    codec_type get_codec_type() const;
    bool is_initialized() const;
    bool is_decoding() const;
    QString get_codec_info() const;
    QString get_last_error() const;

    // Quality monitoring
    quality_metrics get_quality_metrics() const;
    double get_peak_level_db() const;
    double get_rms_level_db() const;
    double get_signal_quality() const;
    bool has_errors() const;
    void clear_error_count();

    // Advanced features
    bool set_gain(double gain); // Audio gain adjustment (0.1-2.0)
    double get_gain() const;
    void enable_error_concealment(bool enabled);
    void enable_quality_monitoring(bool enabled);
    void enable_level_monitoring(bool enabled);

    // Performance monitoring
    uint64_t get_frames_processed() const;
    uint64_t get_bytes_processed() const;
    double get_processing_rate_fps() const;
    std::chrono::microseconds get_average_processing_time() const;

signals:
    /**
     * @brief Emitted when audio frame is successfully decoded
     * @param pcm_data Decoded PCM audio data
     * @param sample_rate Sample rate of decoded data
     * @param channels Number of channels
     */
    void audio_decoded(const QByteArray& pcm_data, uint32_t sample_rate, uint16_t channels);

    /**
     * @brief Emitted when audio frame is decoded to float format
     * @param samples Decoded float samples
     * @param sample_rate Sample rate
     * @param channels Number of channels  
     */
    void audio_decoded_float(const std::vector<float>& samples, uint32_t sample_rate, uint16_t channels);

    /**
     * @brief Emitted when audio levels change (real-time monitoring)
     * @param peak_db Peak level in dBFS
     * @param rms_db RMS level in dBFS
     */
    void audio_levels_changed(double peak_db, double rms_db);

    /**
     * @brief Emitted when codec information changes
     * @param codec Current codec type
     * @param bitrate Bitrate in kbps
     * @param sample_rate Input sample rate
     * @param channels Input channels
     */
    void codec_info_changed(codec_type codec, uint32_t bitrate, uint32_t sample_rate, uint16_t channels);

    /**
     * @brief Emitted when signal quality metrics are updated
     * @param quality Signal quality percentage (0-100)
     * @param bit_error_rate Estimated bit error rate
     */
    void quality_metrics_updated(double quality, double bit_error_rate);

    /**
     * @brief Emitted when decoder error occurs
     * @param error Error description
     * @param recoverable True if decoder can continue
     */
    void decoder_error(const QString& error, bool recoverable);

    /**
     * @brief Emitted when audio clipping is detected
     * @param peak_level Peak level that caused clipping
     */
    void clipping_detected(double peak_level);

    /**
     * @brief Emitted when extended silence is detected
     * @param duration_ms Duration of silence in milliseconds
     */
    void silence_detected(uint32_t duration_ms);

private slots:
    void monitor_audio_levels();
    void update_quality_metrics();

private:
    // Internal implementation
    class AudioDecoderPrivate;
    std::unique_ptr<AudioDecoderPrivate> d_ptr;

    // Implementation methods
    bool initialize_mpeg_decoder();
    bool initialize_aac_decoder(); 
    bool initialize_codec_specific();
    
    bool decode_mpeg_frame(const QByteArray& data, QByteArray& output);
    bool decode_aac_frame(const QByteArray& data, QByteArray& output);
    
    bool convert_to_output_format(const void* input_samples, size_t input_size,
                                 int input_format, QByteArray& output);
    bool convert_to_float_format(const void* input_samples, size_t input_size,
                                int input_format, std::vector<float>& output);
    
    void calculate_audio_levels(const QByteArray& pcm_data);
    void calculate_audio_levels_float(const std::vector<float>& samples);
    void update_signal_quality();
    void detect_audio_issues(const QByteArray& pcm_data);
    
    void cleanup_decoders();
    void reset_metrics();
    void log_error(const QString& error, bool recoverable = true);

    // Static utility methods
    static double linear_to_db(double linear_value);
    static double db_to_linear(double db_value);
    static QString codec_type_to_string(codec_type codec);
    static QString format_sample_rate(uint32_t sample_rate);
};

/**
 * @brief Factory class for creating pre-configured audio decoders
 */
class AudioDecoderFactory {
public:
    /**
     * @brief Create decoder optimized for broadcast monitoring
     */
    static std::unique_ptr<audio_decoder> create_monitoring_decoder();

    /**
     * @brief Create decoder optimized for high-quality audio analysis
     */
    static std::unique_ptr<audio_decoder> create_analysis_decoder();

    /**
     * @brief Create decoder optimized for real-time streaming
     */
    static std::unique_ptr<audio_decoder> create_streaming_decoder();

    /**
     * @brief Create decoder with custom configuration
     */
    static std::unique_ptr<audio_decoder> create_custom_decoder(
        audio_decoder::codec_type codec, 
        const audio_decoder::decoder_config& config);
};

/**
 * @brief Utility functions for audio processing
 */
namespace AudioUtils {
    /**
     * @brief Convert linear amplitude to dB
     */
    double linear_to_db(double linear);

    /**
     * @brief Convert dB to linear amplitude  
     */
    double db_to_linear(double db);

    /**
     * @brief Calculate RMS level from PCM samples
     */
    double calculate_rms_level(const int16_t* samples, size_t count);
    double calculate_rms_level(const float* samples, size_t count);

    /**
     * @brief Calculate peak level from PCM samples
     */
    double calculate_peak_level(const int16_t* samples, size_t count);
    double calculate_peak_level(const float* samples, size_t count);

    /**
     * @brief Detect audio clipping
     */
    bool detect_clipping(const int16_t* samples, size_t count, double threshold = 0.95);
    bool detect_clipping(const float* samples, size_t count, double threshold = 0.95);

    /**
     * @brief Detect extended silence
     */
    bool detect_silence(const int16_t* samples, size_t count, double threshold_db = -40.0);
    bool detect_silence(const float* samples, size_t count, double threshold_db = -40.0);

    /**
     * @brief Apply gain to audio samples
     */
    void apply_gain(int16_t* samples, size_t count, double gain);
    void apply_gain(float* samples, size_t count, double gain);

    /**
     * @brief Convert between sample formats
     */
    void convert_int16_to_float(const int16_t* input, float* output, size_t count);
    void convert_float_to_int16(const float* input, int16_t* output, size_t count);
    void convert_int16_to_int32(const int16_t* input, int32_t* output, size_t count);
    void convert_int32_to_int16(const int32_t* input, int16_t* output, size_t count);

    /**
     * @brief Downmix stereo to mono
     */
    void downmix_stereo_to_mono(const int16_t* stereo_input, int16_t* mono_output, size_t frame_count);
    void downmix_stereo_to_mono(const float* stereo_input, float* mono_output, size_t frame_count);
}

} // namespace eti::audio