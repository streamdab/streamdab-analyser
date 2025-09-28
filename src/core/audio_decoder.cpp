#include "audio_decoder.hpp"
#include "../utils/logger.h"
#include <QDateTime>
#include <QDebug>
#include <QtMath>
#include <algorithm>
#include <numeric>
#include <cmath>
#include <cstring>
#include <chrono>

// Conditional includes for audio libraries
#ifdef USE_FAAD2
extern "C" {
#include <neaacdec.h>
}
#else
// Fallback definitions when FAAD2 is not available
#define FAAD_FMT_16BIT 1
#define FAAD_FMT_FLOAT 7
#endif

#ifdef HAVE_MPG123
#include <mpg123.h>
#endif

namespace eti::audio {

/**
 * @brief Private implementation class (PIMPL pattern)
 */
class audio_decoder::AudioDecoderPrivate {
public:
    AudioDecoderPrivate();
    ~AudioDecoderPrivate();

    // Configuration and status
    decoder_config config;
    decoder_status status;
    mutable QMutex status_mutex;
    
    // Codec handles
#ifdef USE_FAAD2
    NeAACDecHandle faad_handle = nullptr;
    NeAACDecConfigurationPtr faad_config = nullptr;
    NeAACDecFrameInfo faad_frame_info;
#endif
    
#ifdef HAVE_MPG123
    mpg123_handle* mpg123_handle = nullptr;
    int mpg123_encoding = 0;
#endif
    
    // Monitoring and metrics
    QTimer* level_monitor_timer = nullptr;
    QTimer* quality_monitor_timer = nullptr;
    
    // Performance tracking
    uint64_t frames_processed = 0;
    uint64_t bytes_processed = 0;
    std::chrono::steady_clock::time_point start_time;
    std::vector<std::chrono::microseconds> processing_times;
    static constexpr size_t MAX_PROCESSING_TIMES = 100;
    
    // Audio level monitoring
    std::vector<double> peak_history;
    std::vector<double> rms_history;
    static constexpr size_t MAX_LEVEL_HISTORY = 100;
    std::chrono::steady_clock::time_point last_audio_time;
    uint32_t silence_duration_ms = 0;
    
    // Quality metrics tracking
    uint32_t consecutive_errors = 0;
    uint32_t total_error_frames = 0;
    double current_bit_error_rate = 0.0;
    
    // Internal buffers
    std::vector<uint8_t> conversion_buffer;
    std::vector<float> float_buffer;
    
    // Methods
    void reset_codec_handles();
    void update_performance_metrics();
    void add_processing_time(std::chrono::microseconds time);
    void update_level_history(double peak, double rms);
    bool validate_config(const decoder_config& cfg);
    
    // Format conversion methods
    template<typename InputType, typename OutputType>
    void convert_samples(const InputType* input, OutputType* output, size_t count, double gain = 1.0);
    
    void convert_to_int16(const void* input, int input_format, int16_t* output, size_t sample_count, double gain);
    void convert_to_int24(const void* input, int input_format, uint8_t* output, size_t sample_count, double gain);
    void convert_to_float(const void* input, int input_format, float* output, size_t sample_count, double gain);
};

audio_decoder::AudioDecoderPrivate::AudioDecoderPrivate() 
    : start_time(std::chrono::steady_clock::now())
    , last_audio_time(std::chrono::steady_clock::now())
{
    processing_times.reserve(MAX_PROCESSING_TIMES);
    peak_history.reserve(MAX_LEVEL_HISTORY);
    rms_history.reserve(MAX_LEVEL_HISTORY);
    
    // Initialize status
    status.current_codec = codec_type::unknown;
    status.is_initialized = false;
    status.is_decoding = false;
}

audio_decoder::AudioDecoderPrivate::~AudioDecoderPrivate() {
    reset_codec_handles();
}

void audio_decoder::AudioDecoderPrivate::reset_codec_handles() {
#ifdef USE_FAAD2
    if (faad_handle) {
        NeAACDecClose(faad_handle);
        faad_handle = nullptr;
    }
    faad_config = nullptr;
#endif

#ifdef HAVE_MPG123
    if (mpg123_handle) {
        mpg123_close(mpg123_handle);
        mpg123_delete(mpg123_handle);
        mpg123_handle = nullptr;
    }
#endif
}

// Main audio_decoder implementation
audio_decoder::audio_decoder(QObject* parent)
    : QObject(parent)
    , d_ptr(std::make_unique<AudioDecoderPrivate>())
{
    // Initialize audio processing libraries
#ifdef HAVE_MPG123
    static bool mpg123_initialized = false;
    if (!mpg123_initialized) {
        if (mpg123_init() != MPG123_OK) {
            Logger::instance().logError("Failed to initialize mpg123 library", "AudioDecoder");
        } else {
            mpg123_initialized = true;
            Logger::instance().logInfo("mpg123 library initialized", "AudioDecoder");
        }
    }
#endif

    // Setup monitoring timers
    d_ptr->level_monitor_timer = new QTimer(this);
    d_ptr->quality_monitor_timer = new QTimer(this);
    
    connect(d_ptr->level_monitor_timer, &QTimer::timeout, this, &audio_decoder::monitor_audio_levels);
    connect(d_ptr->quality_monitor_timer, &QTimer::timeout, this, &audio_decoder::update_quality_metrics);
    
    Logger::instance().logInfo("Audio decoder created", "AudioDecoder");
}

audio_decoder::~audio_decoder() {
    cleanup_decoders();
    Logger::instance().logInfo("Audio decoder destroyed", "AudioDecoder");
}

bool audio_decoder::initialize(codec_type codec, const decoder_config& config) {
    QMutexLocker locker(&d_ptr->status_mutex);
    
    if (d_ptr->status.is_initialized) {
        cleanup_decoders();
    }
    
    if (!d_ptr->validate_config(config)) {
        log_error("Invalid decoder configuration", false);
        return false;
    }
    
    d_ptr->config = config;
    d_ptr->status.current_codec = codec;
    d_ptr->status.last_error.clear();
    
    bool success = false;
    
    try {
        switch (codec) {
            case codec_type::mpeg1_layer2:
                success = initialize_mpeg_decoder();
                break;
            case codec_type::aac_lc:
            case codec_type::aac_plus:
            case codec_type::aac_plus_v2:
                success = initialize_aac_decoder();
                break;
            default:
                log_error(QString("Unsupported codec type: %1").arg(static_cast<int>(codec)), false);
                return false;
        }
        
        if (success) {
            d_ptr->status.is_initialized = true;
            d_ptr->status.codec_version = get_codec_info();
            d_ptr->start_time = std::chrono::steady_clock::now();
            reset_metrics();
            
            // Start monitoring if enabled
            if (config.enable_level_monitoring && !d_ptr->level_monitor_timer->isActive()) {
                d_ptr->level_monitor_timer->start(100); // Update every 100ms
            }
            
            if (config.enable_quality_monitoring && !d_ptr->quality_monitor_timer->isActive()) {
                d_ptr->quality_monitor_timer->start(1000); // Update every second
            }
            
            emit codec_info_changed(codec, 0, config.sample_rate, config.channels);
            Logger::instance().logInfo(QString("Audio decoder initialized: %1").arg(get_codec_info()), "AudioDecoder");
        }
        
    } catch (const std::exception& e) {
        success = false;
        log_error(QString("Exception during initialization: %1").arg(e.what()), false);
    }
    
    return success;
}

bool audio_decoder::initialize(codec_type codec) {
    return initialize(codec, decoder_config{});
}

bool audio_decoder::initialize_mpeg_decoder() {
#ifdef HAVE_MPG123
    int error = MPG123_OK;
    
    // Create mpg123 handle
    d_ptr->mpg123_handle = mpg123_new(nullptr, &error);
    if (!d_ptr->mpg123_handle || error != MPG123_OK) {
        log_error(QString("Failed to create mpg123 handle: %1").arg(mpg123_plain_strerror(error)), false);
        return false;
    }
    
    // Configure decoder for DAB (no seeking, force rate)
    mpg123_param(d_ptr->mpg123_handle, MPG123_VERBOSE, 0, 0);
    mpg123_param(d_ptr->mpg123_handle, MPG123_ADD_FLAGS, MPG123_FORCE_FLOAT, 0);
    
    if (d_ptr->config.sample_rate > 0) {
        mpg123_param(d_ptr->mpg123_handle, MPG123_FORCE_RATE, d_ptr->config.sample_rate, 0);
    }
    
    // Open feed (streaming mode)
    error = mpg123_open_feed(d_ptr->mpg123_handle);
    if (error != MPG123_OK) {
        log_error(QString("Failed to open mpg123 feed: %1").arg(mpg123_strerror(d_ptr->mpg123_handle)), false);
        mpg123_delete(d_ptr->mpg123_handle);
        d_ptr->mpg123_handle = nullptr;
        return false;
    }
    
    Logger::instance().logInfo("MPEG-1 Layer 2 decoder initialized", "AudioDecoder");
    return true;
#else
    log_error("MPEG-1 Layer 2 support not compiled (missing libmpg123)", false);
    return false;
#endif
}

bool audio_decoder::initialize_aac_decoder() {
#ifdef USE_FAAD2
    // Create FAAD2 handle
    d_ptr->faad_handle = NeAACDecOpen();
    if (!d_ptr->faad_handle) {
        log_error("Failed to create FAAD2 handle", false);
        return false;
    }
    
    // Get and configure FAAD2
    d_ptr->faad_config = NeAACDecGetCurrentConfiguration(d_ptr->faad_handle);
    if (!d_ptr->faad_config) {
        log_error("Failed to get FAAD2 configuration", false);
        NeAACDecClose(d_ptr->faad_handle);
        d_ptr->faad_handle = nullptr;
        return false;
    }
    
    // Configure for DAB+ (optimized settings)
    d_ptr->faad_config->outputFormat = FAAD_FMT_16BIT;
    d_ptr->faad_config->downMatrix = d_ptr->config.enable_downmix ? 1 : 0;
    d_ptr->faad_config->useOldADTSFormat = 0;
    d_ptr->faad_config->dontUpSampleImplicitSBR = 0; // Allow SBR upsampling
    
    // Set configuration
    if (NeAACDecSetConfiguration(d_ptr->faad_handle, d_ptr->faad_config) != 1) {
        log_error("Failed to set FAAD2 configuration", false);
        NeAACDecClose(d_ptr->faad_handle);
        d_ptr->faad_handle = nullptr;
        return false;
    }
    
    Logger::instance().logInfo(QString("AAC decoder initialized: %1")
                              .arg(codec_type_to_string(d_ptr->status.current_codec)), "AudioDecoder");
    return true;
#else
    log_error("AAC support not compiled (missing FAAD2)", false);
    return false;
#endif
}

bool audio_decoder::decode_frame(const QByteArray& encoded_data, QByteArray& decoded_audio) {
    if (!d_ptr->status.is_initialized || encoded_data.isEmpty()) {
        return false;
    }
    
    auto start_time = std::chrono::steady_clock::now();
    bool success = false;
    
    d_ptr->status.is_decoding = true;
    
    try {
        switch (d_ptr->status.current_codec) {
            case codec_type::mpeg1_layer2:
                success = decode_mpeg_frame(encoded_data, decoded_audio);
                break;
            case codec_type::aac_lc:
            case codec_type::aac_plus:
            case codec_type::aac_plus_v2:
                success = decode_aac_frame(encoded_data, decoded_audio);
                break;
            default:
                log_error("Unknown codec type for decoding");
                success = false;
                break;
        }
        
        if (success && !decoded_audio.isEmpty()) {
            // Update metrics
            d_ptr->frames_processed++;
            d_ptr->bytes_processed += decoded_audio.size();
            
            // Calculate audio levels if monitoring enabled
            if (d_ptr->config.enable_level_monitoring) {
                calculate_audio_levels(decoded_audio);
            }
            
            // Detect audio issues
            detect_audio_issues(decoded_audio);
            
            // Emit signals
            emit audio_decoded(decoded_audio, d_ptr->status.output_sample_rate, d_ptr->status.output_channels);
            
        } else if (!success) {
            d_ptr->consecutive_errors++;
            d_ptr->total_error_frames++;
            
            if (d_ptr->consecutive_errors > 10) {
                log_error(QString("Too many consecutive decode errors (%1)").arg(d_ptr->consecutive_errors));
            }
        } else {
            // Reset consecutive errors on successful decode
            d_ptr->consecutive_errors = 0;
        }
        
    } catch (const std::exception& e) {
        success = false;
        log_error(QString("Exception during frame decode: %1").arg(e.what()));
    }
    
    d_ptr->status.is_decoding = false;
    
    // Record processing time
    auto end_time = std::chrono::steady_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time);
    d_ptr->add_processing_time(duration);
    
    return success;
}

bool audio_decoder::decode_mpeg_frame(const QByteArray& data, QByteArray& output) {
#ifdef HAVE_MPG123
    if (!d_ptr->mpg123_handle) {
        return false;
    }
    
    int error = MPG123_OK;
    
    // Feed data to decoder
    error = mpg123_feed(d_ptr->mpg123_handle, 
                       reinterpret_cast<const unsigned char*>(data.constData()), 
                       data.size());
    
    if (error != MPG123_OK && error != MPG123_NEED_MORE) {
        log_error(QString("mpg123_feed error: %1").arg(mpg123_strerror(d_ptr->mpg123_handle)));
        return false;
    }
    
    // Decode audio
    size_t decoded_bytes = 0;
    const int buffer_size = 8192 * 4; // Large enough for any frame
    d_ptr->conversion_buffer.resize(buffer_size);
    
    error = mpg123_read(d_ptr->mpg123_handle, 
                       d_ptr->conversion_buffer.data(), 
                       buffer_size, 
                       &decoded_bytes);
    
    if (error != MPG123_OK && error != MPG123_NEW_FORMAT) {
        if (error != MPG123_NEED_MORE) { // NEED_MORE is common and not an error
            log_error(QString("mpg123_read error: %1").arg(mpg123_strerror(d_ptr->mpg123_handle)));
        }
        return false;
    }
    
    if (decoded_bytes == 0) {
        return false; // No data decoded
    }
    
    // Get format info
    long rate;
    int channels, encoding;
    error = mpg123_getformat(d_ptr->mpg123_handle, &rate, &channels, &encoding);
    
    if (error == MPG123_OK) {
        // Update status if format changed
        if (d_ptr->status.input_sample_rate != static_cast<uint32_t>(rate) ||
            d_ptr->status.input_channels != static_cast<uint16_t>(channels)) {
            
            d_ptr->status.input_sample_rate = rate;
            d_ptr->status.input_channels = channels;
            d_ptr->status.output_sample_rate = rate;
            d_ptr->status.output_channels = channels;
            d_ptr->mpg123_encoding = encoding;
            
            emit codec_info_changed(codec_type::mpeg1_layer2, 0, rate, channels);
            Logger::instance().logInfo(QString("MPEG format: %1Hz %2ch encoding=%3")
                                     .arg(rate).arg(channels).arg(encoding), "AudioDecoder");
        }
    }
    
    // Convert to output format if needed
    if (d_ptr->config.format != output_format::native) {
        return convert_to_output_format(d_ptr->conversion_buffer.data(), decoded_bytes, 
                                       d_ptr->mpg123_encoding, output);
    } else {
        output = QByteArray(reinterpret_cast<const char*>(d_ptr->conversion_buffer.data()), decoded_bytes);
        return true;
    }
#else
    Q_UNUSED(data)
    Q_UNUSED(output)
    return false;
#endif
}

bool audio_decoder::decode_aac_frame(const QByteArray& data, QByteArray& output) {
#ifdef USE_FAAD2
    if (!d_ptr->faad_handle) {
        return false;
    }
    
    // Decode frame
    void* sample_buffer = NeAACDecDecode(d_ptr->faad_handle, 
                                        &d_ptr->faad_frame_info,
                                        reinterpret_cast<unsigned char*>(const_cast<char*>(data.constData())), 
                                        data.size());
    
    if (d_ptr->faad_frame_info.error != 0) {
        QString error_msg = QString("FAAD2 decode error: %1")
                           .arg(NeAACDecGetErrorMessage(d_ptr->faad_frame_info.error));
        
        // Some errors are not critical (e.g., channel coupling warnings)
        if (d_ptr->faad_frame_info.error != 21 && d_ptr->faad_frame_info.error != 12) {
            log_error(error_msg);
        }
        return false;
    }
    
    if (!sample_buffer || d_ptr->faad_frame_info.samples == 0) {
        return false;
    }
    
    // Update status if format changed
    if (d_ptr->status.input_sample_rate != d_ptr->faad_frame_info.samplerate ||
        d_ptr->status.input_channels != d_ptr->faad_frame_info.channels) {
        
        d_ptr->status.input_sample_rate = d_ptr->faad_frame_info.samplerate;
        d_ptr->status.input_channels = d_ptr->faad_frame_info.channels;
        d_ptr->status.output_sample_rate = d_ptr->faad_frame_info.samplerate;
        d_ptr->status.output_channels = d_ptr->faad_frame_info.channels;
        
        emit codec_info_changed(d_ptr->status.current_codec, 0, 
                              d_ptr->faad_frame_info.samplerate, 
                              d_ptr->faad_frame_info.channels);
        
        Logger::instance().logInfo(QString("AAC format: %1Hz %2ch")
                                  .arg(d_ptr->faad_frame_info.samplerate)
                                  .arg(d_ptr->faad_frame_info.channels), "AudioDecoder");
    }
    
    // Calculate output size (FAAD2 outputs 16-bit samples by default)
    size_t output_bytes = d_ptr->faad_frame_info.samples * sizeof(int16_t);
    
    // Convert to output format if needed
    if (d_ptr->config.format != output_format::native) {
        return convert_to_output_format(sample_buffer, output_bytes, FAAD_FMT_16BIT, output);
    } else {
        output = QByteArray(reinterpret_cast<const char*>(sample_buffer), output_bytes);
        return true;
    }
#else
    Q_UNUSED(data)
    Q_UNUSED(output)
    return false;
#endif
}

bool audio_decoder::convert_to_output_format(const void* input_samples, size_t input_size,
                                            int input_format, QByteArray& output) {
    if (!input_samples || input_size == 0) {
        return false;
    }
    
    size_t sample_count = 0;
    size_t output_bytes = 0;
    
    // Determine input format and calculate sample count
#ifdef HAVE_MPG123
    if (input_format == MPG123_ENC_SIGNED_16) {
        sample_count = input_size / sizeof(int16_t);
    } else if (input_format == MPG123_ENC_FLOAT_32) {
        sample_count = input_size / sizeof(float);
    }
#endif
    
#ifdef USE_FAAD2
    if (input_format == FAAD_FMT_16BIT) {
        sample_count = input_size / sizeof(int16_t);
    } else if (input_format == FAAD_FMT_FLOAT) {
        sample_count = input_size / sizeof(float);
    }
#endif
    
    if (sample_count == 0) {
        return false;
    }
    
    // Calculate output size based on target format
    switch (d_ptr->config.format) {
        case output_format::pcm_16_le:
            output_bytes = sample_count * sizeof(int16_t);
            break;
        case output_format::pcm_24_le:
            output_bytes = sample_count * 3; // 24-bit = 3 bytes
            break;
        case output_format::pcm_32_float:
            output_bytes = sample_count * sizeof(float);
            break;
        case output_format::native:
            output_bytes = input_size;
            break;
    }
    
    output.resize(output_bytes);
    
    // Perform conversion
    switch (d_ptr->config.format) {
        case output_format::pcm_16_le:
            d_ptr->convert_to_int16(input_samples, input_format, 
                                   reinterpret_cast<int16_t*>(output.data()), 
                                   sample_count, d_ptr->config.gain_adjustment);
            break;
            
        case output_format::pcm_32_float:
            d_ptr->convert_to_float(input_samples, input_format,
                                   reinterpret_cast<float*>(output.data()),
                                   sample_count, d_ptr->config.gain_adjustment);
            break;
            
        case output_format::pcm_24_le:
            d_ptr->convert_to_int24(input_samples, input_format,
                                   reinterpret_cast<uint8_t*>(output.data()),
                                   sample_count, d_ptr->config.gain_adjustment);
            break;
            
        case output_format::native:
            // Just copy data for native format
            std::memcpy(output.data(), input_samples, input_size);
            if (d_ptr->config.gain_adjustment != 1.0) {
                // Apply gain to native format
                if (input_format == FAAD_FMT_16BIT) {
                    AudioUtils::apply_gain(reinterpret_cast<int16_t*>(output.data()), 
                                         sample_count, d_ptr->config.gain_adjustment);
                }
            }
            break;
    }
    
    return true;
}

void audio_decoder::AudioDecoderPrivate::convert_to_int16(const void* input, int input_format, 
                                                         int16_t* output, size_t sample_count, double gain) {
    if (input_format == FAAD_FMT_16BIT) {
        // Input is already int16_t
        const int16_t* input_samples = static_cast<const int16_t*>(input);
        if (gain == 1.0) {
            std::memcpy(output, input_samples, sample_count * sizeof(int16_t));
        } else {
            for (size_t i = 0; i < sample_count; ++i) {
                double sample = static_cast<double>(input_samples[i]) * gain;
                output[i] = static_cast<int16_t>(qBound(-32768.0, sample, 32767.0));
            }
        }
    }
#ifdef HAVE_MPG123
    else if (input_format == MPG123_ENC_FLOAT_32) {
        // Convert float to int16_t
        const float* input_samples = static_cast<const float*>(input);
        for (size_t i = 0; i < sample_count; ++i) {
            double sample = static_cast<double>(input_samples[i]) * 32767.0 * gain;
            output[i] = static_cast<int16_t>(qBound(-32768.0, sample, 32767.0));
        }
    }
#endif
}

void audio_decoder::AudioDecoderPrivate::convert_to_float(const void* input, int input_format,
                                                          float* output, size_t sample_count, double gain) {
    if (input_format == FAAD_FMT_16BIT) {
        // Convert int16_t to float
        const int16_t* input_samples = static_cast<const int16_t*>(input);
        for (size_t i = 0; i < sample_count; ++i) {
            output[i] = static_cast<float>(input_samples[i]) / 32767.0f * static_cast<float>(gain);
        }
    }
#ifdef HAVE_MPG123
    else if (input_format == MPG123_ENC_FLOAT_32) {
        // Input is already float
        const float* input_samples = static_cast<const float*>(input);
        if (gain == 1.0) {
            std::memcpy(output, input_samples, sample_count * sizeof(float));
        } else {
            for (size_t i = 0; i < sample_count; ++i) {
                output[i] = input_samples[i] * static_cast<float>(gain);
            }
        }
    }
#endif
}

void audio_decoder::AudioDecoderPrivate::convert_to_int24(const void* input, int input_format,
                                                          uint8_t* output, size_t sample_count, double gain) {
    if (input_format == FAAD_FMT_16BIT) {
        // Convert int16_t to 24-bit (stored as 3 bytes per sample, little-endian)
        const int16_t* input_samples = static_cast<const int16_t*>(input);
        for (size_t i = 0; i < sample_count; ++i) {
            // Scale 16-bit sample to 24-bit range with gain
            int32_t sample_32 = static_cast<int32_t>(input_samples[i] * gain);
            sample_32 = sample_32 << 8; // Scale from 16-bit to 24-bit range
            
            // Clamp to 24-bit signed range (-8,388,608 to 8,388,607)
            if (sample_32 > 8388607) {
                sample_32 = 8388607;
            } else if (sample_32 < -8388608) {
                sample_32 = -8388608;
            }
            
            // Store as 3 bytes in little-endian format
            size_t byte_index = i * 3;
            output[byte_index] = static_cast<uint8_t>(sample_32 & 0xFF);         // LSB
            output[byte_index + 1] = static_cast<uint8_t>((sample_32 >> 8) & 0xFF);  // Middle byte
            output[byte_index + 2] = static_cast<uint8_t>((sample_32 >> 16) & 0xFF); // MSB
        }
    }
#ifdef HAVE_MPG123
    else if (input_format == MPG123_ENC_FLOAT_32) {
        // Convert float to 24-bit
        const float* input_samples = static_cast<const float*>(input);
        for (size_t i = 0; i < sample_count; ++i) {
            // Scale float sample to 24-bit range with gain
            float scaled_sample = input_samples[i] * static_cast<float>(gain);
            int32_t sample_32 = static_cast<int32_t>(scaled_sample * 8388607.0f);
            
            // Clamp to 24-bit signed range
            if (sample_32 > 8388607) {
                sample_32 = 8388607;
            } else if (sample_32 < -8388608) {
                sample_32 = -8388608;
            }
            
            // Store as 3 bytes in little-endian format
            size_t byte_index = i * 3;
            output[byte_index] = static_cast<uint8_t>(sample_32 & 0xFF);         // LSB
            output[byte_index + 1] = static_cast<uint8_t>((sample_32 >> 8) & 0xFF);  // Middle byte
            output[byte_index + 2] = static_cast<uint8_t>((sample_32 >> 16) & 0xFF); // MSB
        }
    }
#endif
    else {
        // Unsupported input format for 24-bit conversion
        // Fill with silence
        std::memset(output, 0, sample_count * 3);
    }
}

void audio_decoder::calculate_audio_levels(const QByteArray& pcm_data) {
    if (pcm_data.isEmpty()) {
        return;
    }
    
    // Assume 16-bit PCM for now
    const int16_t* samples = reinterpret_cast<const int16_t*>(pcm_data.constData());
    size_t sample_count = pcm_data.size() / sizeof(int16_t);
    
    if (sample_count == 0) {
        return;
    }
    
    // Calculate peak and RMS levels
    double peak_linear = AudioUtils::calculate_peak_level(samples, sample_count);
    double rms_linear = AudioUtils::calculate_rms_level(samples, sample_count);
    
    double peak_db = AudioUtils::linear_to_db(peak_linear);
    double rms_db = AudioUtils::linear_to_db(rms_linear);
    
    // Update status
    d_ptr->status.metrics.peak_level_linear = peak_linear;
    d_ptr->status.metrics.rms_level_linear = rms_linear;
    d_ptr->status.metrics.peak_level_db = peak_db;
    d_ptr->status.metrics.rms_level_db = rms_db;
    
    // Update history
    d_ptr->update_level_history(peak_linear, rms_linear);
    
    // Check for clipping
    if (peak_linear > 0.95) {
        d_ptr->status.metrics.clipping_detected = true;
        emit clipping_detected(peak_db);
    } else {
        d_ptr->status.metrics.clipping_detected = false;
    }
    
    // Update last audio time
    d_ptr->last_audio_time = std::chrono::steady_clock::now();
    
    // Emit level change signal
    emit audio_levels_changed(peak_db, rms_db);
}

void audio_decoder::detect_audio_issues(const QByteArray& pcm_data) {
    if (pcm_data.isEmpty()) {
        return;
    }
    
    const int16_t* samples = reinterpret_cast<const int16_t*>(pcm_data.constData());
    size_t sample_count = pcm_data.size() / sizeof(int16_t);
    
    // Detect silence
    bool is_silent = AudioUtils::detect_silence(samples, sample_count, -40.0);
    
    if (is_silent) {
        auto now = std::chrono::steady_clock::now();
        auto silence_duration = std::chrono::duration_cast<std::chrono::milliseconds>(
            now - d_ptr->last_audio_time);
        
        d_ptr->silence_duration_ms = silence_duration.count();
        
        if (d_ptr->silence_duration_ms > 5000) { // 5 seconds of silence
            d_ptr->status.metrics.silence_detected = true;
            emit silence_detected(d_ptr->silence_duration_ms);
        }
    } else {
        d_ptr->status.metrics.silence_detected = false;
        d_ptr->silence_duration_ms = 0;
        d_ptr->last_audio_time = std::chrono::steady_clock::now();
    }
}

void audio_decoder::monitor_audio_levels() {
    // This slot is called by timer to emit regular level updates
    if (d_ptr->config.enable_level_monitoring && !d_ptr->peak_history.empty()) {
        double current_peak = d_ptr->peak_history.back();
        double current_rms = d_ptr->rms_history.back();
        
        double peak_db = AudioUtils::linear_to_db(current_peak);
        double rms_db = AudioUtils::linear_to_db(current_rms);
        
        emit audio_levels_changed(peak_db, rms_db);
    }
}

void audio_decoder::update_quality_metrics() {
    if (!d_ptr->config.enable_quality_monitoring) {
        return;
    }
    
    // Calculate signal quality based on error rate and processing success
    double error_rate = 0.0;
    if (d_ptr->frames_processed > 0) {
        error_rate = static_cast<double>(d_ptr->total_error_frames) / d_ptr->frames_processed;
    }
    
    double signal_quality = 100.0 * (1.0 - std::min(error_rate, 1.0));
    
    d_ptr->status.metrics.bit_error_rate = error_rate;
    d_ptr->status.metrics.signal_quality = signal_quality;
    d_ptr->current_bit_error_rate = error_rate;
    
    emit quality_metrics_updated(signal_quality, error_rate);
}

// Status and information methods
audio_decoder::decoder_status audio_decoder::get_status() const {
    QMutexLocker locker(&d_ptr->status_mutex);
    return d_ptr->status;
}

bool audio_decoder::is_initialized() const {
    QMutexLocker locker(&d_ptr->status_mutex);
    return d_ptr->status.is_initialized;
}

QString audio_decoder::get_codec_info() const {
    switch (d_ptr->status.current_codec) {
        case codec_type::mpeg1_layer2:
#ifdef HAVE_MPG123
            return QString("MPEG-1 Layer 2 (libmpg123 %1)").arg(mpg123_distversion());
#else
            return "MPEG-1 Layer 2 (not available)";
#endif
        case codec_type::aac_lc:
            return "AAC-LC (FAAD2)";
        case codec_type::aac_plus:
#ifdef USE_FAAD2
            return QString("HE-AAC v1 (FAAD2 %1)").arg(FAAD2_VERSION);
#else
            return "HE-AAC v1 (not available)";
#endif
        case codec_type::aac_plus_v2:
#ifdef USE_FAAD2
            return QString("HE-AAC v2 (FAAD2 %1)").arg(FAAD2_VERSION);
#else
            return "HE-AAC v2 (not available)";
#endif
        default:
            return "Unknown";
    }
}

// Utility methods
void audio_decoder::cleanup_decoders() {
    QMutexLocker locker(&d_ptr->status_mutex);
    
    d_ptr->level_monitor_timer->stop();
    d_ptr->quality_monitor_timer->stop();
    
    d_ptr->reset_codec_handles();
    d_ptr->status.is_initialized = false;
    d_ptr->status.is_decoding = false;
    
    Logger::instance().logInfo("Audio decoder cleanup completed", "AudioDecoder");
}

void audio_decoder::reset_metrics() {
    d_ptr->status.metrics = quality_metrics{};
    d_ptr->frames_processed = 0;
    d_ptr->bytes_processed = 0;
    d_ptr->consecutive_errors = 0;
    d_ptr->total_error_frames = 0;
    d_ptr->processing_times.clear();
    d_ptr->peak_history.clear();
    d_ptr->rms_history.clear();
}

void audio_decoder::log_error(const QString& error, bool recoverable) {
    d_ptr->status.last_error = error;
    Logger::instance().logError(error, "AudioDecoder");
    emit decoder_error(error, recoverable);
}

QString audio_decoder::codec_type_to_string(codec_type codec) {
    switch (codec) {
        case codec_type::mpeg1_layer2: return "MPEG-1 Layer 2";
        case codec_type::aac_lc: return "AAC-LC";
        case codec_type::aac_plus: return "HE-AAC v1";
        case codec_type::aac_plus_v2: return "HE-AAC v2";
        default: return "Unknown";
    }
}

// Static utility function implementation
double audio_decoder::linear_to_db(double linear_value) {
    if (linear_value <= 0.0) {
        return -60.0; // Minimum dB value
    }
    return 20.0 * std::log10(linear_value);
}

// AudioUtils implementation
namespace AudioUtils {

double linear_to_db(double linear) {
    if (linear <= 0.0) {
        return -60.0; // Minimum dB value
    }
    return 20.0 * std::log10(linear);
}

double db_to_linear(double db) {
    return std::pow(10.0, db / 20.0);
}

double calculate_rms_level(const int16_t* samples, size_t count) {
    if (!samples || count == 0) return 0.0;
    
    double sum = 0.0;
    for (size_t i = 0; i < count; ++i) {
        double sample = static_cast<double>(samples[i]) / 32767.0;
        sum += sample * sample;
    }
    
    return std::sqrt(sum / count);
}

double calculate_peak_level(const int16_t* samples, size_t count) {
    if (!samples || count == 0) return 0.0;
    
    double max_sample = 0.0;
    for (size_t i = 0; i < count; ++i) {
        double sample = std::abs(static_cast<double>(samples[i])) / 32767.0;
        max_sample = std::max(max_sample, sample);
    }
    
    return max_sample;
}

bool detect_silence(const int16_t* samples, size_t count, double threshold_db) {
    double rms = calculate_rms_level(samples, count);
    double rms_db = linear_to_db(rms);
    return rms_db < threshold_db;
}

void apply_gain(int16_t* samples, size_t count, double gain) {
    if (!samples || gain == 1.0) return;
    
    for (size_t i = 0; i < count; ++i) {
        double sample = static_cast<double>(samples[i]) * gain;
        samples[i] = static_cast<int16_t>(qBound(-32768.0, sample, 32767.0));
    }
}

} // namespace AudioUtils

} // namespace eti::audio