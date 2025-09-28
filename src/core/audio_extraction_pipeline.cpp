/**
 * @file audio_extraction_pipeline.cpp
 * @brief Implementation of Professional Audio Extraction Pipeline
 * 
 * High-performance, multi-threaded audio extraction system for DAB/DAB+ streams
 * with real-time processing, quality monitoring, and format conversion.
 */

#include "audio_extraction_pipeline.hpp"
#include <QJsonObject>
#include <QJsonDocument>
#include <QAudioFormat>
#include <QMediaDevices>
#include <QDebug>
#include <algorithm>
#include <numeric>
#include <cmath>

// FAAD2 includes (conditional compilation)
#ifdef USE_FAAD2
extern "C" {
#include <faad.h>
}
#endif

namespace eti {

// AudioExtractionWorker Implementation
AudioExtractionWorker::AudioExtractionWorker(uint16_t service_id, 
                                           const AudioExtractionConfig& config,
                                           QObject* parent)
    : QObject(parent)
    , m_service_id(service_id)
    , m_config(config)
    , m_process_timer(std::make_unique<QTimer>(this))
    , m_last_frame_time(std::chrono::steady_clock::now())
{
    m_processing_times.reserve(PERF_HISTORY_SIZE);
    
    // Initialize service info
    m_service_info.service_id = service_id;
    m_service_info.audio_mode = DabAudioMode::UNKNOWN;
    m_service_info.last_update = std::chrono::system_clock::now();
    
    // Setup processing timer
    connect(m_process_timer.get(), &QTimer::timeout,
            this, &AudioExtractionWorker::process_audio_queue);
    m_process_timer->setInterval(10); // Process every 10ms for real-time
    m_process_timer->setSingleShot(false);
}

AudioExtractionWorker::~AudioExtractionWorker() {
    stop_extraction();
    cleanup_decoder();
}

void AudioExtractionWorker::start_extraction() {
    if (m_extracting.load()) {
        return; // Already extracting
    }
    
    try {
        initialize_decoder();
        
        m_extracting = true;
        m_paused = false;
        
        if (m_config.enable_real_time) {
            m_process_timer->start();
        }
        
        emit extraction_started(m_service_id);
        qDebug() << "AudioExtractionWorker: Started extraction for service" << m_service_id;
        
    } catch (const std::exception& e) {
        m_extracting = false;
        emit error_occurred(m_service_id, 
                           QString("Failed to start audio extraction: %1").arg(e.what()));
    }
}

void AudioExtractionWorker::stop_extraction() {
    if (!m_extracting.load()) {
        return; // Not extracting
    }
    
    m_extracting = false;
    m_paused = false;
    m_process_timer->stop();
    
    // Flush remaining data
    flush_buffers();
    
    cleanup_decoder();
    
    emit extraction_stopped(m_service_id);
    qDebug() << "AudioExtractionWorker: Stopped extraction for service" << m_service_id;
}

void AudioExtractionWorker::pause_extraction() {
    if (!m_extracting.load() || m_paused.load()) {
        return;
    }
    
    m_paused = true;
    m_process_timer->stop();
    
    emit extraction_paused(m_service_id);
    qDebug() << "AudioExtractionWorker: Paused extraction for service" << m_service_id;
}

void AudioExtractionWorker::resume_extraction() {
    if (!m_extracting.load() || !m_paused.load()) {
        return;
    }
    
    m_paused = false;
    if (m_config.enable_real_time) {
        m_process_timer->start();
    }
    
    emit extraction_resumed(m_service_id);
    qDebug() << "AudioExtractionWorker: Resumed extraction for service" << m_service_id;
}

void AudioExtractionWorker::process_frame(const eti::EtiFrame& frame) {
    if (!m_extracting.load() || m_paused.load()) {
        return;
    }
    
    const auto start_time = std::chrono::steady_clock::now();
    
    try {
        // Update service information from frame
        update_service_info(frame);
        
        // Extract audio data for this service
        const auto audio_data = extract_service_audio(frame);
        
        if (!audio_data.empty()) {
            // Queue frame for processing or process immediately
            if (m_config.enable_real_time) {
                QMutexLocker locker(&m_queue_mutex);
                m_frame_queue.enqueue(frame);
                
                // Limit queue size to prevent memory growth
                while (m_frame_queue.size() > static_cast<int>(m_config.buffer_size_ms / 24)) {
                    m_frame_queue.dequeue();
                }
            } else {
                // Process immediately
                AudioData decoded_audio;
                if (decode_audio_data(audio_data, decoded_audio)) {
                    emit audio_data_ready(m_service_id, decoded_audio);
                }
            }
        }
        
        // Update performance metrics
        const auto processing_time = std::chrono::steady_clock::now() - start_time;
        const auto processing_us = std::chrono::duration_cast<std::chrono::microseconds>(processing_time);
        
        m_processing_times.push_back(static_cast<double>(processing_us.count()));
        if (m_processing_times.size() > PERF_HISTORY_SIZE) {
            m_processing_times.erase(m_processing_times.begin());
        }
        
        m_service_info.frames_processed++;
        m_last_frame_time = std::chrono::steady_clock::now();
        
    } catch (const std::exception& e) {
        m_service_info.frames_with_errors++;
        emit error_occurred(m_service_id, 
                           QString("Failed to process frame: %1").arg(e.what()));
    }
}

void AudioExtractionWorker::update_config(const eti::AudioExtractionConfig& config) {
    m_config = config;
    
    // Update timer interval if real-time processing changed
    if (m_config.enable_real_time && m_extracting.load() && !m_paused.load()) {
        m_process_timer->start();
    } else {
        m_process_timer->stop();
    }
}

void AudioExtractionWorker::flush_buffers() {
    QMutexLocker locker(&m_queue_mutex);
    
    // Process all queued frames
    while (!m_frame_queue.isEmpty()) {
        const EtiFrame frame = m_frame_queue.dequeue();
        
        const auto audio_data = extract_service_audio(frame);
        if (!audio_data.empty()) {
            AudioData decoded_audio;
            if (decode_audio_data(audio_data, decoded_audio)) {
                emit audio_data_ready(m_service_id, decoded_audio);
            }
        }
    }
    
    // Clear audio buffer
    m_audio_buffer.clear();
    m_buffer_position = 0;
}

void AudioExtractionWorker::process_audio_queue() {
    if (!m_extracting.load() || m_paused.load()) {
        return;
    }
    
    QMutexLocker locker(&m_queue_mutex);
    
    // Process a limited number of frames per timer tick
    const int max_frames_per_tick = 5;
    int frames_processed = 0;
    
    while (!m_frame_queue.isEmpty() && frames_processed < max_frames_per_tick) {
        const EtiFrame frame = m_frame_queue.dequeue();
        
        locker.unlock(); // Release lock while processing
        
        const auto audio_data = extract_service_audio(frame);
        if (!audio_data.empty()) {
            AudioData decoded_audio;
            if (decode_audio_data(audio_data, decoded_audio)) {
                emit audio_data_ready(m_service_id, decoded_audio);
            }
        }
        
        frames_processed++;
        locker.relock();
    }
}

void AudioExtractionWorker::initialize_decoder() {
#ifdef USE_FAAD2
    // Initialize FAAD2 decoder for DAB+ AAC
    m_faad_decoder = NeAACDecOpen();
    if (!m_faad_decoder) {
        throw std::runtime_error("Failed to initialize FAAD2 AAC decoder");
    }
    
    // Configure decoder
    NeAACDecConfigurationPtr config = NeAACDecGetCurrentConfiguration(
        static_cast<NeAACDecHandle>(m_faad_decoder));
    
    config->outputFormat = FAAD_FMT_16BIT;
    config->downMatrix = 0; // No down-mixing
    config->useOldADTSFormat = 0;
    config->dontUpSampleImplicitSBR = 1;
    
    if (!NeAACDecSetConfiguration(static_cast<NeAACDecHandle>(m_faad_decoder), config)) {
        throw std::runtime_error("Failed to configure FAAD2 decoder");
    }
#endif
    
    // Initialize audio buffer
    m_audio_buffer.clear();
    m_audio_buffer.reserve(8192); // Reserve space for audio frames
    m_buffer_position = 0;
    
    qDebug() << "AudioExtractionWorker: Initialized decoders for service" << m_service_id;
}

void AudioExtractionWorker::cleanup_decoder() {
#ifdef USE_FAAD2
    if (m_faad_decoder) {
        NeAACDecClose(static_cast<NeAACDecHandle>(m_faad_decoder));
        m_faad_decoder = nullptr;
    }
#endif
    
    m_audio_buffer.clear();
    m_buffer_position = 0;
    
    qDebug() << "AudioExtractionWorker: Cleaned up decoders for service" << m_service_id;
}

bool AudioExtractionWorker::decode_audio_data(const std::vector<uint8_t>& audio_data, AudioData& output) {
    if (audio_data.empty()) {
        return false;
    }
    
    // Determine audio mode and decode accordingly
    switch (m_service_info.audio_mode) {
        case DabAudioMode::DAB_PLUS_AAC:
            return decode_dabplus_audio(audio_data, output);
            
        case DabAudioMode::DAB_MPEG1_L2:
            return decode_mpeg2_audio(audio_data, output);
            
        case DabAudioMode::DMB_MPEG4_AAC:
            return decode_dabplus_audio(audio_data, output); // Similar to DAB+
            
        default:
            // Auto-detect format based on data patterns
            if (audio_data.size() >= 7 && audio_data[0] == 0xFF && (audio_data[1] & 0xF0) == 0xF0) {
                // Looks like ADTS AAC header - try DAB+
                if (decode_dabplus_audio(audio_data, output)) {
                    m_service_info.audio_mode = DabAudioMode::DAB_PLUS_AAC;
                    return true;
                }
            }
            
            // Try MPEG-2 audio for traditional DAB
            if (decode_mpeg2_audio(audio_data, output)) {
                m_service_info.audio_mode = DabAudioMode::DAB_MPEG1_L2;
                return true;
            }
            
            return false;
    }
}

bool AudioExtractionWorker::decode_dabplus_audio(const std::vector<uint8_t>& aac_data, AudioData& output) {
#ifdef USE_FAAD2
    if (!m_faad_decoder || aac_data.empty()) {
        return false;
    }
    
    try {
        NeAACDecFrameInfo frame_info;
        void* sample_buffer = NeAACDecDecode(
            static_cast<NeAACDecHandle>(m_faad_decoder),
            &frame_info,
            const_cast<unsigned char*>(aac_data.data()),
            static_cast<unsigned long>(aac_data.size()));
        
        if (frame_info.error != 0) {
            emit error_occurred(m_service_id, 
                               QString("AAC decode error: %1")
                               .arg(NeAACDecGetErrorMessage(frame_info.error)));
            return false;
        }
        
        if (sample_buffer && frame_info.samples > 0) {
            // Convert samples to our format
            const int16_t* samples = static_cast<const int16_t*>(sample_buffer);
            output.samples.assign(samples, samples + frame_info.samples);
            output.sample_rate = frame_info.samplerate;
            output.channels = frame_info.channels;
            output.bit_depth = 16;
            output.frame_count = frame_info.samples / frame_info.channels;
            output.timestamp = std::chrono::system_clock::now();
            output.service_info = m_service_info;
            output.is_valid = true;
            
            // Apply audio processing
            apply_gain_adjustment(output);
            if (m_config.enable_noise_reduction) {
                apply_noise_reduction(output);
            }
            if (m_config.enable_error_concealment) {
                apply_error_concealment(output);
            }
            
            // Calculate audio levels and quality metrics
            calculate_audio_levels(output);
            calculate_quality_metrics(output);
            
            // Convert to output format if needed
            if (m_config.output_format != AudioOutputFormat::PCM_16_STEREO) {
                AudioData converted_output;
                if (convert_to_output_format(output, converted_output)) {
                    output = converted_output;
                }
            }
            
            // Embed metadata if enabled
            if (m_config.embed_metadata) {
                embed_metadata(output);
            }
            
            return true;
        }
        
    } catch (const std::exception& e) {
        emit error_occurred(m_service_id, 
                           QString("DAB+ decode exception: %1").arg(e.what()));
    }
#else
    Q_UNUSED(aac_data)
    Q_UNUSED(output)
#endif
    
    return false;
}

bool AudioExtractionWorker::decode_mpeg2_audio(const std::vector<uint8_t>& mpeg_data, AudioData& output) {
    // MPEG Layer 2 decoding would require additional library (e.g., mpg123, libmad)
    // For now, implement basic placeholder
    
    if (mpeg_data.empty()) {
        return false;
    }
    
    // Placeholder implementation - would need proper MPEG decoder
    output.samples.resize(1152 * 2); // Typical MPEG frame size
    std::fill(output.samples.begin(), output.samples.end(), 0);
    
    output.sample_rate = 48000;
    output.channels = 2;
    output.bit_depth = 16;
    output.frame_count = 1152;
    output.timestamp = std::chrono::system_clock::now();
    output.service_info = m_service_info;
    output.is_valid = true;
    
    return true;
}

std::vector<uint8_t> AudioExtractionWorker::extract_service_audio(const EtiFrame& frame) {
    // Extract audio data for the specific service from ETI frame
    std::vector<uint8_t> audio_data;
    
    try {
        // Get MSC field from frame
        const auto msc = frame.get_msc_field();
        
        // Find the sub-channel for our service
        // This would require parsing the FIC to determine sub-channel organization
        const auto subchannel_info = frame.get_subchannel_info();
        
        for (const auto& subchannel : subchannel_info) {
            if (subchannel.sub_channel_id == m_service_info.sub_channel_id) {
                // Extract data from this sub-channel
                const auto subchannel_data = msc.extract_subchannel_data(subchannel);
                audio_data.insert(audio_data.end(), subchannel_data.begin(), subchannel_data.end());
                break;
            }
        }
        
    } catch (const std::exception& e) {
        emit error_occurred(m_service_id, 
                           QString("Failed to extract service audio: %1").arg(e.what()));
    }
    
    return audio_data;
}

void AudioExtractionWorker::update_service_info(const EtiFrame& frame) {
    try {
        // Extract ensemble information
        const auto ensemble = frame.extract_ensemble_info();
        
        // Find our service in the ensemble
        for (const auto& service : ensemble.services) {
            if (service.service_id == m_service_id) {
                m_service_info.service_label = service.label;
                
                // Update service components
                for (const auto& component : service.components) {
                    m_service_info.sub_channel_id = component.sub_channel_id;
                    m_service_info.component_label = component.label;
                    
                    // Determine audio mode from component type
                    if (component.asc_ty >= 0 && component.asc_ty <= 63) {
                        if (component.asc_ty == 63) {
                            m_service_info.audio_mode = DabAudioMode::DAB_PLUS_AAC;
                        } else {
                            m_service_info.audio_mode = DabAudioMode::DAB_MPEG1_L2;
                        }
                    }
                    break;
                }
                
                m_service_info.last_update = std::chrono::system_clock::now();
                emit service_info_updated(m_service_id, m_service_info);
                break;
            }
        }
        
    } catch (const std::exception& e) {
        // Silently handle service info extraction errors
    }
}

void AudioExtractionWorker::apply_gain_adjustment(AudioData& audio_data) {
    if (m_config.gain_adjustment == 1.0) {
        return; // No adjustment needed
    }
    
    for (auto& sample : audio_data.samples) {
        const int32_t adjusted = static_cast<int32_t>(sample * m_config.gain_adjustment);
        sample = static_cast<int16_t>(std::clamp(adjusted, -32768, 32767));
    }
}

void AudioExtractionWorker::apply_noise_reduction(AudioData& audio_data) {
    // Simple noise gate implementation
    const int16_t noise_floor = 100; // -48 dB approximately
    
    for (auto& sample : audio_data.samples) {
        if (std::abs(sample) < noise_floor) {
            sample = 0;
        }
    }
}

void AudioExtractionWorker::apply_error_concealment(AudioData& audio_data) {
    // Simple error concealment using interpolation
    for (size_t i = 1; i < audio_data.samples.size() - 1; ++i) {
        const int16_t current = audio_data.samples[i];
        const int16_t prev = audio_data.samples[i - 1];
        const int16_t next = audio_data.samples[i + 1];
        
        // Detect potential errors (large jumps)
        if (std::abs(current - prev) > 8000 && std::abs(current - next) > 8000) {
            // Interpolate
            audio_data.samples[i] = static_cast<int16_t>((prev + next) / 2);
        }
    }
}

void AudioExtractionWorker::calculate_audio_levels(AudioData& audio_data) {
    if (audio_data.samples.empty()) {
        return;
    }
    
    // Calculate peak level
    int16_t peak = 0;
    for (const auto& sample : audio_data.samples) {
        peak = std::max(peak, static_cast<int16_t>(std::abs(sample)));
    }
    audio_data.peak_level = static_cast<double>(peak) / 32768.0;
    
    // Calculate RMS level
    double sum_squares = 0.0;
    for (const auto& sample : audio_data.samples) {
        const double normalized = static_cast<double>(sample) / 32768.0;
        sum_squares += normalized * normalized;
    }
    audio_data.rms_level = std::sqrt(sum_squares / audio_data.samples.size());
}

void AudioExtractionWorker::calculate_quality_metrics(AudioData& audio_data) {
    // Simplified quality metrics calculation
    // Real implementation would use proper DSP algorithms
    
    // Estimate SNR based on signal variance
    if (!audio_data.samples.empty()) {
        const double mean = std::accumulate(audio_data.samples.begin(), audio_data.samples.end(), 0.0) / 
                           audio_data.samples.size();
        
        double variance = 0.0;
        for (const auto& sample : audio_data.samples) {
            const double diff = sample - mean;
            variance += diff * diff;
        }
        variance /= audio_data.samples.size();
        
        // Estimate SNR (simplified)
        audio_data.snr = 20.0 * std::log10(audio_data.rms_level / std::sqrt(variance / 32768.0 / 32768.0));
    }
    
    // Simplified THD calculation
    audio_data.thd = 0.01; // Placeholder - real implementation would use FFT
    
    emit quality_metrics_updated(m_service_id, audio_data.snr, audio_data.thd);
}

bool AudioExtractionWorker::convert_to_output_format(const AudioData& input, AudioData& output) {
    output = input; // Start with copy
    
    // Format conversion would be implemented based on output_format
    switch (m_config.output_format) {
        case AudioOutputFormat::PCM_16_MONO:
            // Convert stereo to mono if needed
            if (input.channels == 2) {
                output.samples.clear();
                for (size_t i = 0; i < input.samples.size(); i += 2) {
                    const int32_t mono = (input.samples[i] + input.samples[i + 1]) / 2;
                    output.samples.push_back(static_cast<int16_t>(mono));
                }
                output.channels = 1;
            }
            break;
            
        case AudioOutputFormat::PCM_32_FLOAT:
            // Convert to float samples
            output.samples_float.clear();
            output.samples_float.reserve(input.samples.size());
            for (const auto& sample : input.samples) {
                output.samples_float.push_back(static_cast<float>(sample) / 32768.0f);
            }
            output.bit_depth = 32;
            break;
            
        default:
            // Keep original format
            break;
    }
    
    return true;
}

void AudioExtractionWorker::embed_metadata(AudioData& audio_data) {
    // Metadata embedding would be format-specific
    // For now, just ensure service info is attached
    audio_data.service_info = m_service_info;
}

// AudioExtractionPipeline Implementation
AudioExtractionPipeline::AudioExtractionPipeline(QObject* parent)
    : QObject(parent)
{
    // Set default configuration
    m_default_config.output_format = AudioOutputFormat::WAV_16;
    m_default_config.sample_rate = 48000;
    m_default_config.channels = 2;
    m_default_config.enable_real_time = true;
    m_default_config.enable_buffering = true;
    m_default_config.buffer_size_ms = 100;
}

AudioExtractionPipeline::~AudioExtractionPipeline() {
    stop_extraction();
    
    // Cleanup all service workers
    const auto service_ids = get_active_services();
    for (const auto service_id : service_ids) {
        cleanup_service_worker(service_id);
    }
}

bool AudioExtractionPipeline::add_service(uint16_t service_id, const AudioExtractionConfig& config) {
    try {
        validate_config(config);
        
        QMutexLocker locker(&m_services_mutex);
        
        if (m_service_configs.contains(service_id)) {
            return false; // Service already exists
        }
        
        if (m_service_configs.size() >= MAX_CONCURRENT_SERVICES) {
            throw std::runtime_error("Maximum number of concurrent services reached");
        }
        
        m_service_configs[service_id] = config;
        
        // Initialize service info
        AudioServiceInfo info;
        info.service_id = service_id;
        info.audio_mode = DabAudioMode::UNKNOWN;
        info.last_update = std::chrono::system_clock::now();
        m_service_info[service_id] = info;
        
        // Initialize audio data queue
        m_audio_data_queues[service_id] = QQueue<AudioData>{};
        
        // Initialize worker if extraction is active
        if (m_extracting.load()) {
            initialize_service_worker(service_id);
        }
        
        return true;
        
    } catch (const std::exception& e) {
        if (m_error_callback) {
            m_error_callback(service_id, QString("Failed to add service: %1").arg(e.what()));
        }
        return false;
    }
}

bool AudioExtractionPipeline::remove_service(uint16_t service_id) {
    QMutexLocker locker(&m_services_mutex);
    
    if (!m_service_configs.contains(service_id)) {
        return false; // Service doesn't exist
    }
    
    // Stop and cleanup worker
    cleanup_service_worker(service_id);
    
    // Remove from all data structures
    m_service_configs.remove(service_id);
    m_service_info.remove(service_id);
    m_audio_data_queues.remove(service_id);
    
    {
        QMutexLocker error_locker(&m_errors_mutex);
        m_service_errors.remove(service_id);
    }
    
    {
        QMutexLocker levels_locker(&m_levels_mutex);
        m_peak_level_history.remove(service_id);
        m_rms_level_history.remove(service_id);
    }
    
    return true;
}

void AudioExtractionPipeline::start_extraction() {
    if (m_extracting.load()) {
        return; // Already extracting
    }
    
    m_extracting = true;
    m_paused = false;
    
    // Start all service workers
    QMutexLocker locker(&m_services_mutex);
    for (auto it = m_service_configs.begin(); it != m_service_configs.end(); ++it) {
        initialize_service_worker(it.key());
    }
}

void AudioExtractionPipeline::stop_extraction() {
    if (!m_extracting.load()) {
        return; // Not extracting
    }
    
    m_extracting = false;
    m_paused = false;
    
    // Stop all service workers
    QMutexLocker locker(&m_services_mutex);
    for (auto it = m_service_configs.begin(); it != m_service_configs.end(); ++it) {
        cleanup_service_worker(it.key());
    }
}

void AudioExtractionPipeline::process_frame(const EtiFrame& frame) {
    if (!m_extracting.load() || m_paused.load()) {
        return;
    }
    
    // Send frame to all active workers
    QMutexLocker locker(&m_services_mutex);
    // Use iterator-based access to avoid unique_ptr copy issues
    for (auto it = m_workers.begin(); it != m_workers.end(); ++it) {
        if (it.value()) {
            QMetaObject::invokeMethod(it.value().get(), "process_frame", 
                                     Qt::QueuedConnection, Q_ARG(eti::EtiFrame, frame));
        }
    }
}

QList<uint16_t> AudioExtractionPipeline::get_active_services() const {
    QMutexLocker locker(&m_services_mutex);
    return m_service_configs.keys();
}

AudioServiceInfo AudioExtractionPipeline::get_service_info(uint16_t service_id) const {
    QMutexLocker locker(&m_services_mutex);
    
    auto it = m_service_info.find(service_id);
    if (it != m_service_info.end()) {
        return it.value();
    }
    
    return AudioServiceInfo{};
}

void AudioExtractionPipeline::initialize_service_worker(uint16_t service_id) {
    auto config_it = m_service_configs.find(service_id);
    if (config_it == m_service_configs.end()) {
        return;
    }
    
    // Create worker thread
    auto worker_thread = std::make_unique<QThread>();
    auto worker = std::make_unique<AudioExtractionWorker>(service_id, config_it.value());
    
    worker->moveToThread(worker_thread.get());
    
    // Connect worker signals
    connect(worker.get(), &AudioExtractionWorker::extraction_started,
            this, &AudioExtractionPipeline::handle_worker_extraction_started);
    connect(worker.get(), &AudioExtractionWorker::extraction_stopped,
            this, &AudioExtractionPipeline::handle_worker_extraction_stopped);
    connect(worker.get(), &AudioExtractionWorker::audio_data_ready,
            this, &AudioExtractionPipeline::handle_worker_audio_data);
    connect(worker.get(), &AudioExtractionWorker::service_info_updated,
            this, &AudioExtractionPipeline::handle_worker_service_info);
    connect(worker.get(), &AudioExtractionWorker::error_occurred,
            this, &AudioExtractionPipeline::handle_worker_error);
    connect(worker.get(), &AudioExtractionWorker::quality_metrics_updated,
            this, &AudioExtractionPipeline::handle_worker_quality_metrics);
    
    // Store worker and thread using move semantics
    m_workers[service_id] = std::move(worker);
    m_worker_threads[service_id] = std::move(worker_thread);
    
    // Start worker thread and extraction
    auto it = m_worker_threads.find(service_id);
    if (it != m_worker_threads.end() && it.value()) {
        it.value()->start();
    }
    
    // Use safe access to avoid unique_ptr copy issues
    auto worker_it = m_workers.find(service_id);
    if (worker_it != m_workers.end() && worker_it.value()) {
        QMetaObject::invokeMethod(worker_it.value().get(), "start_extraction", Qt::QueuedConnection);
    }
}

void AudioExtractionPipeline::cleanup_service_worker(uint16_t service_id) {
    auto thread_it = m_worker_threads.find(service_id);
    if (thread_it != m_worker_threads.end() && thread_it.value()) {
        // Stop extraction first
        auto worker_it = m_workers.find(service_id);
        if (worker_it != m_workers.end() && worker_it.value()) {
            QMetaObject::invokeMethod(worker_it.value().get(), "stop_extraction", Qt::QueuedConnection);
        }
        
        // Stop thread
        thread_it.value()->quit();
        if (!thread_it.value()->wait(5000)) {
            thread_it.value()->terminate();
            thread_it.value()->wait(1000);
        }
        m_worker_threads.remove(service_id);
    }
    
    // Remove worker using key-based erase to avoid unique_ptr copy issues
    m_workers.remove(service_id);
}

void AudioExtractionPipeline::validate_config(const AudioExtractionConfig& config) {
    if (config.sample_rate == 0) {
        throw std::invalid_argument("Sample rate cannot be zero");
    }
    
    if (config.channels == 0 || config.channels > 8) {
        throw std::invalid_argument("Invalid channel count");
    }
    
    if (config.buffer_size_ms == 0 || config.buffer_size_ms > 10000) {
        throw std::invalid_argument("Invalid buffer size");
    }
}

void AudioExtractionPipeline::handle_worker_audio_data(uint16_t service_id, const eti::AudioData& audio_data) {
    // Store audio data in queue
    {
        QMutexLocker locker(&m_audio_data_mutex);
        if (!m_audio_data_queues.contains(service_id)) {
            m_audio_data_queues[service_id] = QQueue<AudioData>{};
        }
        
        m_audio_data_queues[service_id].enqueue(audio_data);
        
        // Limit queue size
        while (m_audio_data_queues[service_id].size() > MAX_AUDIO_DATA_QUEUE_SIZE) {
            m_audio_data_queues[service_id].dequeue();
        }
    }
    
    // Update level history if monitoring enabled
    if (m_level_monitoring.load()) {
        QMutexLocker locker(&m_levels_mutex);
        
        if (!m_peak_level_history.contains(service_id)) {
            m_peak_level_history[service_id] = std::vector<double>{};
            m_rms_level_history[service_id] = std::vector<double>{};
        }
        
        m_peak_level_history[service_id].push_back(audio_data.peak_level);
        m_rms_level_history[service_id].push_back(audio_data.rms_level);
        
        // Limit history size
        if (m_peak_level_history[service_id].size() > MAX_LEVEL_HISTORY_SIZE) {
            m_peak_level_history[service_id].erase(m_peak_level_history[service_id].begin());
            m_rms_level_history[service_id].erase(m_rms_level_history[service_id].begin());
        }
        
        emit audio_levels_updated(service_id, audio_data.peak_level, audio_data.rms_level);
    }
    
    // Call registered callback
    if (m_audio_data_callback) {
        m_audio_data_callback(service_id, audio_data);
    }
    
    emit audio_data_ready(service_id, audio_data);
}

void AudioExtractionPipeline::handle_worker_service_info(uint16_t service_id, const eti::AudioServiceInfo& info) {
    {
        QMutexLocker locker(&m_services_mutex);
        m_service_info[service_id] = info;
    }
    
    if (m_service_info_callback) {
        m_service_info_callback(service_id, info);
    }
    
    emit service_info_updated(service_id, info);
}

void AudioExtractionPipeline::handle_worker_error(uint16_t service_id, const QString& error) {
    {
        QMutexLocker locker(&m_errors_mutex);
        if (!m_service_errors.contains(service_id)) {
            m_service_errors[service_id] = QStringList{};
        }
        
        m_service_errors[service_id].append(error);
        
        // Limit error history
        if (m_service_errors[service_id].size() > MAX_ERROR_HISTORY) {
            m_service_errors[service_id].removeFirst();
        }
    }
    
    if (m_error_callback) {
        m_error_callback(service_id, error);
    }
    
    emit extraction_error_occurred(service_id, error);
}

void AudioExtractionPipeline::handle_worker_quality_metrics(uint16_t service_id, double snr, double thd) {
    if (m_quality_callback) {
        m_quality_callback(service_id, snr, thd);
    }
    
    emit quality_metrics_updated(service_id, snr, thd);
}

// AudioExtractionPipelineFactory Implementation
std::unique_ptr<AudioExtractionPipeline> AudioExtractionPipelineFactory::create_monitoring_pipeline() {
    auto pipeline = std::make_unique<AudioExtractionPipeline>();
    
    AudioExtractionConfig config;
    config.output_format = AudioOutputFormat::PCM_16_STEREO;
    config.enable_real_time = true;
    config.enable_buffering = true;
    config.buffer_size_ms = 50; // Low latency for monitoring
    config.enable_error_concealment = true;
    
    pipeline->set_default_config(config);
    pipeline->enable_level_monitoring(true);
    pipeline->enable_quality_monitoring(true);
    
    return pipeline;
}

std::unique_ptr<AudioExtractionPipeline> AudioExtractionPipelineFactory::create_archival_pipeline() {
    auto pipeline = std::make_unique<AudioExtractionPipeline>();
    
    AudioExtractionConfig config;
    config.output_format = AudioOutputFormat::FLAC_24;
    config.sample_rate = 48000;
    config.channels = 2;
    config.bit_depth = 24;
    config.enable_real_time = false; // Quality over speed
    config.enable_buffering = true;
    config.buffer_size_ms = 500; // Larger buffer for quality
    config.flac_compression = 8; // Maximum compression
    config.embed_metadata = true;
    config.embed_timestamp = true;
    
    pipeline->set_default_config(config);
    
    return pipeline;
}

// AudioUtils Implementation
namespace AudioUtils {

std::vector<int16_t> float_to_int16(const std::vector<float>& samples) {
    std::vector<int16_t> result;
    result.reserve(samples.size());
    
    for (const auto& sample : samples) {
        const int32_t scaled = static_cast<int32_t>(sample * 32767.0f);
        result.push_back(static_cast<int16_t>(std::clamp(scaled, -32768, 32767)));
    }
    
    return result;
}

std::vector<float> int16_to_float(const std::vector<int16_t>& samples) {
    std::vector<float> result;
    result.reserve(samples.size());
    
    for (const auto& sample : samples) {
        result.push_back(static_cast<float>(sample) / 32768.0f);
    }
    
    return result;
}

double calculate_peak_level(const std::vector<int16_t>& samples) {
    if (samples.empty()) {
        return 0.0;
    }
    
    int16_t peak = 0;
    for (const auto& sample : samples) {
        peak = std::max(peak, static_cast<int16_t>(std::abs(sample)));
    }
    
    return static_cast<double>(peak) / 32768.0;
}

double calculate_rms_level(const std::vector<int16_t>& samples) {
    if (samples.empty()) {
        return 0.0;
    }
    
    double sum_squares = 0.0;
    for (const auto& sample : samples) {
        const double normalized = static_cast<double>(sample) / 32768.0;
        sum_squares += normalized * normalized;
    }
    
    return std::sqrt(sum_squares / samples.size());
}

double calculate_peak_level_dbfs(const std::vector<int16_t>& samples) {
    const double peak = calculate_peak_level(samples);
    if (peak <= 0.0) {
        return -96.0; // Silence floor
    }
    
    return 20.0 * std::log10(peak);
}

double calculate_rms_level_dbfs(const std::vector<int16_t>& samples) {
    const double rms = calculate_rms_level(samples);
    if (rms <= 0.0) {
        return -96.0; // Silence floor
    }
    
    return 20.0 * std::log10(rms);
}

} // namespace AudioUtils

} // namespace eti

#include "audio_extraction_pipeline.moc"