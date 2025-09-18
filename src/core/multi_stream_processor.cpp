/**
 * @file multi_stream_processor.cpp
 * @brief Implementation of Professional Multi-Stream Processor
 * 
 * High-performance, scalable processor for concurrent ETI streams with
 * load balancing, failover, and professional broadcast workflow integration.
 */

#include "multi_stream_processor.h"
#include <QJsonObject>
#include <QJsonDocument>
#include <QJsonArray>
#include <QNetworkInterface>
#include <QHostAddress>
#include <QCoreApplication>
#include <QProcess>
#include <QFileInfo>
#include <QDebug>
#include <algorithm>
#include <numeric>
#include <thread>

namespace eti {

// StreamProcessingTask Implementation
StreamProcessingTask::StreamProcessingTask(const QString& stream_id, const EtiFrame& frame,
                                         const StreamConfig& config)
    : m_stream_id(stream_id)
    , m_frame(frame)
    , m_config(config)
{
    setAutoDelete(true);
}

void StreamProcessingTask::run() {
    const auto start_time = std::chrono::steady_clock::now();
    
    try {
        // Validate frame quality
        bool frame_valid = m_frame.is_valid();
        
        if (frame_valid) {
            // Perform stream-specific processing based on mode
            switch (m_config.processing_mode) {
                case StreamProcessingMode::REAL_TIME:
                    // Minimal processing for real-time
                    break;
                    
                case StreamProcessingMode::ANALYSIS:
                    // Detailed analysis processing
                    break;
                    
                case StreamProcessingMode::MONITORING:
                    // Quality monitoring processing
                    break;
                    
                default:
                    break;
            }
        }
        
        const auto processing_time = std::chrono::steady_clock::now() - start_time;
        const auto processing_us = std::chrono::duration_cast<std::chrono::microseconds>(processing_time);
        
        if (m_result_callback) {
            const QString result = frame_valid ? "OK" : "INVALID_FRAME";
            m_result_callback(m_stream_id, frame_valid, result);
        }
        
    } catch (const std::exception& e) {
        if (m_result_callback) {
            m_result_callback(m_stream_id, false, QString("Processing error: %1").arg(e.what()));
        }
    }
}

void StreamProcessingTask::set_result_callback(std::function<void(const QString&, bool, const QString&)> callback) {
    m_result_callback = callback;
}

// StreamWorker Implementation
StreamWorker::StreamWorker(const StreamConfig& config, QObject* parent)
    : QObject(parent)
    , m_config(config)
    , m_statistics_timer(std::make_unique<QTimer>(this))
    , m_failover_timer(std::make_unique<QTimer>(this))
    , m_last_frame_time(std::chrono::steady_clock::now())
{
    m_frame_rate_history.reserve(PERF_HISTORY_SIZE);
    m_latency_history.reserve(PERF_HISTORY_SIZE);
    
    // Initialize status
    m_status.stream_id = m_config.stream_id;
    m_status.reset();
    
    // Setup timers
    connect(m_statistics_timer.get(), &QTimer::timeout,
            this, &StreamWorker::update_statistics);
    m_statistics_timer->setInterval(1000); // Update every second
    m_statistics_timer->setSingleShot(false);
    
    connect(m_failover_timer.get(), &QTimer::timeout,
            this, &StreamWorker::attempt_failover);
    m_failover_timer->setSingleShot(true);
}

StreamWorker::~StreamWorker() {
    stop_stream();
    cleanup_components();
}

void StreamWorker::start_stream() {
    if (m_active.load()) {
        return; // Already active
    }
    
    try {
        initialize_components();
        
        m_active = true;
        m_paused = false;
        m_status.is_active = true;
        m_status.reset();
        
        // Start components
        if (m_receiver) {
            m_receiver->start_reception();
        }
        
        m_statistics_timer->start();
        
        emit stream_started(m_config.stream_id);
        qDebug() << "StreamWorker: Started stream" << m_config.stream_id;
        
    } catch (const std::exception& e) {
        m_active = false;
        m_status.is_active = false;
        emit error_occurred(m_config.stream_id, 
                           QString("Failed to start stream: %1").arg(e.what()));
    }
}

void StreamWorker::stop_stream() {
    if (!m_active.load()) {
        return; // Not active
    }
    
    m_active = false;
    m_paused = false;
    m_statistics_timer->stop();
    m_failover_timer->stop();
    
    // Stop components
    if (m_receiver) {
        m_receiver->stop_reception();
    }
    
    if (m_recording_manager) {
        const auto active_sessions = m_recording_manager->get_active_sessions();
        for (const auto& session_id : active_sessions) {
            m_recording_manager->stop_recording_session(session_id);
        }
    }
    
    if (m_audio_pipeline) {
        m_audio_pipeline->stop_extraction();
    }
    
    m_status.is_active = false;
    m_status.is_connected = false;
    m_status.is_processing = false;
    m_status.is_recording = false;
    
    cleanup_components();
    
    emit stream_stopped(m_config.stream_id);
    qDebug() << "StreamWorker: Stopped stream" << m_config.stream_id;
}

void StreamWorker::pause_stream() {
    if (!m_active.load() || m_paused.load()) {
        return;
    }
    
    m_paused = true;
    
    if (m_receiver) {
        m_receiver->pause_reception();
    }
    
    if (m_audio_pipeline) {
        m_audio_pipeline->pause_extraction();
    }
    
    m_status.is_processing = false;
    
    emit stream_paused(m_config.stream_id);
    qDebug() << "StreamWorker: Paused stream" << m_config.stream_id;
}

void StreamWorker::resume_stream() {
    if (!m_active.load() || !m_paused.load()) {
        return;
    }
    
    m_paused = false;
    
    if (m_receiver) {
        m_receiver->resume_reception();
    }
    
    if (m_audio_pipeline) {
        m_audio_pipeline->resume_extraction();
    }
    
    m_status.is_processing = true;
    
    emit stream_resumed(m_config.stream_id);
    qDebug() << "StreamWorker: Resumed stream" << m_config.stream_id;
}

void StreamWorker::update_config(const eti::StreamConfig& config) {
    const bool was_active = m_active.load();
    
    if (was_active) {
        stop_stream();
    }
    
    m_config = config;
    
    if (was_active) {
        start_stream();
    }
}

void StreamWorker::handle_frame_received(const eti::EtiFrame& frame) {
    if (!m_active.load() || m_paused.load()) {
        return;
    }
    
    QMutexLocker locker(&m_queue_mutex);
    m_frame_queue.enqueue(frame);
    
    // Limit queue size to prevent memory growth
    while (m_frame_queue.size() > static_cast<int>(m_config.buffer_size)) {
        m_frame_queue.dequeue();
        m_status.frames_dropped++;
    }
    
    // Process frame immediately for real-time mode
    if (m_config.processing_mode == StreamProcessingMode::REAL_TIME) {
        locker.unlock();
        process_frame_internal(frame);
    }
}

void StreamWorker::update_statistics() {
    if (!m_active.load()) {
        return;
    }
    
    QMutexLocker locker(&m_status_mutex);
    
    // Update frame rate
    if (!m_frame_rate_history.empty()) {
        const double average_rate = std::accumulate(
            m_frame_rate_history.begin(), m_frame_rate_history.end(), 0.0) / 
            m_frame_rate_history.size();
        m_status.average_frame_rate = average_rate;
    }
    
    // Update latency statistics
    if (!m_latency_history.empty()) {
        const auto average_latency = std::accumulate(
            m_latency_history.begin(), m_latency_history.end(), std::chrono::microseconds{0}) /
            m_latency_history.size();
        m_status.average_latency_us = average_latency.count();
        
        const auto max_latency = *std::max_element(m_latency_history.begin(), m_latency_history.end());
        m_status.max_latency_us = max_latency.count();
    }
    
    // Calculate frame loss rate
    const size_t total_expected = m_status.frames_received + m_status.frames_dropped;
    if (total_expected > 0) {
        m_status.frame_loss_rate = static_cast<double>(m_status.frames_dropped) / total_expected;
    }
    
    // Update timestamp
    m_status.last_frame_time = std::chrono::system_clock::now();
    
    // Check quality thresholds
    check_quality_thresholds();
    
    emit statistics_updated(m_config.stream_id, m_status);
}

void StreamWorker::initialize_components() {
    setup_receiver();
    
    if (m_config.enable_recording) {
        setup_recording();
    }
    
    if (m_config.enable_audio_extraction) {
        setup_audio_extraction();
    }
}

void StreamWorker::cleanup_components() {
    m_receiver.reset();
    m_recording_manager.reset();
    m_audio_pipeline.reset();
}

void StreamWorker::setup_receiver() {
    switch (m_config.source_type) {
        case StreamSourceType::MULTICAST_UDP: {
            NetworkStreamConfig receiver_config;
            receiver_config.multicast_address = m_config.multicast_address;
            receiver_config.port = m_config.port;
            receiver_config.interface_name = m_config.interface_name;
            receiver_config.buffer_size = m_config.buffer_size;
            receiver_config.timeout = m_config.timeout;
            receiver_config.min_frame_rate = m_config.min_frame_rate;
            receiver_config.max_frame_loss = m_config.max_frame_loss;
            receiver_config.max_latency = m_config.max_latency;
            receiver_config.auto_reconnect = m_config.enable_failover;
            
            m_receiver = std::make_unique<NetworkStreamReceiver>(receiver_config);
            
            // Connect receiver signals
            connect(m_receiver.get(), &NetworkStreamReceiver::frame_received,
                    this, &StreamWorker::handle_receiver_frame);
            connect(m_receiver.get(), &NetworkStreamReceiver::error_occurred,
                    this, &StreamWorker::handle_receiver_error);
            connect(m_receiver.get(), &NetworkStreamReceiver::statistics_updated,
                    this, &StreamWorker::handle_receiver_statistics);
            
            break;
        }
        
        case StreamSourceType::FILE_PLAYBACK:
            // File playback would be implemented here
            break;
            
        default:
            throw std::runtime_error("Unsupported stream source type");
    }
}

void StreamWorker::setup_recording() {
    m_recording_manager = std::make_unique<StreamRecordingManager>();
    
    // Start recording session
    SegmentationConfig seg_config;
    seg_config.segment_duration = std::chrono::minutes{60};
    seg_config.enable_compression = false;
    seg_config.auto_cleanup = true;
    
    const QString session_id = m_recording_manager->start_recording_session(
        m_config.output_directory, m_config.recording_format, seg_config);
    
    if (!session_id.isEmpty()) {
        m_status.is_recording = true;
        qDebug() << "StreamWorker: Started recording session" << session_id 
                 << "for stream" << m_config.stream_id;
    }
}

void StreamWorker::setup_audio_extraction() {
    m_audio_pipeline = std::make_unique<AudioExtractionPipeline>();
    
    // Configure audio extraction
    AudioExtractionConfig audio_config;
    audio_config.output_format = m_config.audio_format;
    audio_config.enable_real_time = (m_config.processing_mode == StreamProcessingMode::REAL_TIME);
    audio_config.enable_buffering = true;
    audio_config.buffer_size_ms = 100;
    
    m_audio_pipeline->set_default_config(audio_config);
    m_audio_pipeline->start_extraction();
    
    qDebug() << "StreamWorker: Started audio extraction for stream" << m_config.stream_id;
}

void StreamWorker::process_frame_internal(const EtiFrame& frame) {
    const auto start_time = std::chrono::steady_clock::now();
    
    try {
        // Validate frame quality
        if (!validate_frame_quality(frame)) {
            m_status.processing_errors++;
            return;
        }
        
        // Update performance metrics
        update_performance_metrics(frame);
        
        // Send to recording if enabled
        if (m_recording_manager && m_status.is_recording) {
            const auto active_sessions = m_recording_manager->get_active_sessions();
            for (const auto& session_id : active_sessions) {
                m_recording_manager->record_frame(session_id, frame);
            }
        }
        
        // Send to audio extraction if enabled
        if (m_audio_pipeline) {
            m_audio_pipeline->process_frame(frame);
        }
        
        m_status.frames_processed++;
        
        emit frame_processed(m_config.stream_id, frame);
        
    } catch (const std::exception& e) {
        m_status.processing_errors++;
        emit error_occurred(m_config.stream_id, 
                           QString("Frame processing error: %1").arg(e.what()));
    }
    
    // Update latency metrics
    const auto processing_time = std::chrono::steady_clock::now() - start_time;
    const auto latency = std::chrono::duration_cast<std::chrono::microseconds>(processing_time);
    
    m_latency_history.push_back(latency);
    if (m_latency_history.size() > PERF_HISTORY_SIZE) {
        m_latency_history.erase(m_latency_history.begin());
    }
}

void StreamWorker::update_performance_metrics(const EtiFrame& frame) {
    const auto now = std::chrono::steady_clock::now();
    const auto frame_duration = now - m_last_frame_time;
    const auto frame_duration_us = std::chrono::duration_cast<std::chrono::microseconds>(frame_duration);
    
    if (frame_duration_us.count() > 0) {
        const double instantaneous_rate = 1000000.0 / frame_duration_us.count();
        
        m_frame_rate_history.push_back(instantaneous_rate);
        if (m_frame_rate_history.size() > PERF_HISTORY_SIZE) {
            m_frame_rate_history.erase(m_frame_rate_history.begin());
        }
        
        m_status.current_frame_rate = instantaneous_rate;
    }
    
    m_last_frame_time = now;
    m_status.frames_received++;
    m_status.bytes_received += frame.size();
}

void StreamWorker::check_quality_thresholds() {
    // Check frame rate threshold
    if (m_status.average_frame_rate < m_config.min_frame_rate) {
        const QString violation = QString("Frame rate below threshold: %1 < %2")
                                 .arg(m_status.average_frame_rate.load())
                                 .arg(m_config.min_frame_rate);
        emit quality_threshold_violated(m_config.stream_id, violation);
        
        // Trigger failover if enabled
        if (m_config.enable_failover && !m_failover_in_progress.load()) {
            m_failover_timer->start(m_config.failover_delay.count());
        }
    }
    
    // Check frame loss threshold
    if (m_status.frame_loss_rate > m_config.max_frame_loss) {
        const QString violation = QString("Frame loss above threshold: %1% > %2%")
                                 .arg(m_status.frame_loss_rate.load() * 100, 0, 'f', 2)
                                 .arg(m_config.max_frame_loss * 100, 0, 'f', 2);
        emit quality_threshold_violated(m_config.stream_id, violation);
    }
    
    // Check latency threshold
    if (m_status.average_latency_us > m_config.max_latency.count() * 1000) {
        const QString violation = QString("Latency above threshold: %1µs > %2µs")
                                 .arg(m_status.average_latency_us.load())
                                 .arg(m_config.max_latency.count() * 1000);
        emit quality_threshold_violated(m_config.stream_id, violation);
    }
}

bool StreamWorker::validate_frame_quality(const EtiFrame& frame) {
    // Basic frame validation
    if (!frame.is_valid()) {
        return false;
    }
    
    // Check sync pattern
    if (!frame.validate_sync_pattern()) {
        return false;
    }
    
    // Additional quality checks based on processing mode
    switch (m_config.processing_mode) {
        case StreamProcessingMode::ANALYSIS:
            // Detailed quality analysis
            return frame.validate_frame_structure();
            
        default:
            return true;
    }
}

void StreamWorker::handle_receiver_frame(const eti::EtiFrame& frame) {
    handle_frame_received(frame);
}

void StreamWorker::handle_receiver_error(const QString& error) {
    emit error_occurred(m_config.stream_id, error);
}

void StreamWorker::handle_receiver_statistics(const eti::NetworkStreamStatistics& stats) {
    // Update status from receiver statistics
    m_status.frames_received = stats.frames_received;
    m_status.frames_dropped = stats.frames_dropped;
    m_status.bytes_received = stats.bytes_received;
    m_status.current_frame_rate = stats.current_frame_rate;
    m_status.average_frame_rate = stats.average_frame_rate;
    m_status.frame_loss_rate = stats.frame_loss_rate;
    m_status.average_latency_us = stats.average_latency_us;
    m_status.max_latency_us = stats.max_latency_us;
    m_status.is_connected = true;
}

void StreamWorker::attempt_failover() {
    if (!m_config.enable_failover || m_config.backup_addresses.isEmpty()) {
        return;
    }
    
    m_failover_in_progress = true;
    
    const size_t backup_index = m_current_backup_index.load();
    if (backup_index < static_cast<size_t>(m_config.backup_addresses.size())) {
        const QString backup_address = m_config.backup_addresses[static_cast<int>(backup_index)];
        
        // Update configuration to use backup address
        StreamConfig failover_config = m_config;
        failover_config.multicast_address = backup_address;
        
        // Stop current receiver
        if (m_receiver) {
            m_receiver->stop_reception();
        }
        
        // Create new receiver with backup address
        try {
            setup_receiver();
            m_receiver->start_reception();
            
            emit failover_triggered(m_config.stream_id, backup_address);
            qDebug() << "StreamWorker: Failover to" << backup_address << "for stream" << m_config.stream_id;
            
        } catch (const std::exception& e) {
            emit error_occurred(m_config.stream_id, 
                               QString("Failover failed: %1").arg(e.what()));
        }
        
        // Move to next backup for future failovers
        m_current_backup_index = (backup_index + 1) % m_config.backup_addresses.size();
    }
    
    m_failover_in_progress = false;
}

// MultiStreamProcessor Implementation
MultiStreamProcessor::MultiStreamProcessor(QObject* parent)
    : QObject(parent)
    , m_thread_pool(std::make_unique<QThreadPool>(this))
    , m_statistics_timer(std::make_unique<QTimer>(this))
    , m_resource_monitor_timer(std::make_unique<QTimer>(this))
    , m_load_balancing_timer(std::make_unique<QTimer>(this))
{
    initialize_thread_pool();
    
    // Setup timers
    connect(m_statistics_timer.get(), &QTimer::timeout,
            this, &MultiStreamProcessor::update_global_statistics);
    m_statistics_timer->setInterval(1000);
    m_statistics_timer->start();
    
    connect(m_resource_monitor_timer.get(), &QTimer::timeout,
            this, &MultiStreamProcessor::check_resource_usage);
    m_resource_monitor_timer->setInterval(5000); // Check every 5 seconds
    m_resource_monitor_timer->start();
    
    connect(m_load_balancing_timer.get(), &QTimer::timeout,
            this, &MultiStreamProcessor::perform_load_balancing);
    m_load_balancing_timer->setInterval(10000); // Balance every 10 seconds
    m_load_balancing_timer->start();
}

MultiStreamProcessor::~MultiStreamProcessor() {
    stop_all_streams();
    cleanup_thread_pool();
}

bool MultiStreamProcessor::add_stream(const StreamConfig& config) {
    try {
        validate_stream_config(config);
        
        if (!can_add_stream()) {
            throw std::runtime_error("Maximum number of concurrent streams reached");
        }
        
        QWriteLocker locker(&m_streams_lock);
        
        QString stream_id = config.stream_id;
        if (stream_id.isEmpty() || m_stream_configs.contains(stream_id)) {
            stream_id = generate_unique_stream_id(config.description);
        }
        
        StreamConfig final_config = config;
        final_config.stream_id = stream_id;
        
        m_stream_configs[stream_id] = final_config;
        
        StreamStatus status;
        status.stream_id = stream_id;
        status.reset();
        m_stream_status[stream_id] = status;
        
        emit stream_added(stream_id);
        
        return true;
        
    } catch (const std::exception& e) {
        if (m_error_callback) {
            m_error_callback("", QString("Failed to add stream: %1").arg(e.what()));
        }
        return false;
    }
}

bool MultiStreamProcessor::remove_stream(const QString& stream_id) {
    QWriteLocker locker(&m_streams_lock);
    
    if (!m_stream_configs.contains(stream_id)) {
        return false;
    }
    
    // Stop stream if active
    if (m_workers.contains(stream_id)) {
        cleanup_stream_worker(stream_id);
    }
    
    // Remove from all data structures
    m_stream_configs.remove(stream_id);
    m_stream_status.remove(stream_id);
    
    {
        QMutexLocker error_locker(&m_errors_mutex);
        m_stream_errors.remove(stream_id);
    }
    
    emit stream_removed(stream_id);
    
    return true;
}

bool MultiStreamProcessor::start_stream(const QString& stream_id) {
    QReadLocker locker(&m_streams_lock);
    
    if (!m_stream_configs.contains(stream_id)) {
        return false;
    }
    
    if (m_workers.contains(stream_id)) {
        return true; // Already started
    }
    
    try {
        initialize_stream_worker(stream_id);
        return true;
        
    } catch (const std::exception& e) {
        if (m_error_callback) {
            m_error_callback(stream_id, QString("Failed to start stream: %1").arg(e.what()));
        }
        return false;
    }
}

bool MultiStreamProcessor::stop_stream(const QString& stream_id) {
    QReadLocker locker(&m_streams_lock);
    
    if (!m_workers.contains(stream_id)) {
        return false; // Not running
    }
    
    cleanup_stream_worker(stream_id);
    return true;
}

void MultiStreamProcessor::start_all_streams() {
    QReadLocker locker(&m_streams_lock);
    
    for (auto it = m_stream_configs.begin(); it != m_stream_configs.end(); ++it) {
        if (!m_workers.contains(it.key())) {
            initialize_stream_worker(it.key());
        }
    }
}

void MultiStreamProcessor::stop_all_streams() {
    QReadLocker locker(&m_streams_lock);
    
    for (auto it = m_workers.begin(); it != m_workers.end(); ++it) {
        cleanup_stream_worker(it.key());
    }
}

QStringList MultiStreamProcessor::get_active_streams() const {
    QReadLocker locker(&m_streams_lock);
    
    QStringList active_streams;
    for (auto it = m_stream_status.begin(); it != m_stream_status.end(); ++it) {
        if (it->is_active) {
            active_streams.append(it.key());
        }
    }
    
    return active_streams;
}

QStringList MultiStreamProcessor::get_all_streams() const {
    QReadLocker locker(&m_streams_lock);
    return m_stream_configs.keys();
}

StreamStatus MultiStreamProcessor::get_stream_status(const QString& stream_id) const {
    QReadLocker locker(&m_streams_lock);
    
    auto it = m_stream_status.find(stream_id);
    if (it != m_stream_status.end()) {
        return it.value();
    }
    
    return StreamStatus{};
}

size_t MultiStreamProcessor::get_active_stream_count() const {
    return static_cast<size_t>(get_active_streams().size());
}

double MultiStreamProcessor::get_total_frame_rate() const {
    return m_total_frame_rate.load();
}

void MultiStreamProcessor::initialize_thread_pool() {
    const int cpu_count = std::thread::hardware_concurrency();
    const size_t optimal_threads = std::max(4, std::min(cpu_count * 2, static_cast<int>(DEFAULT_THREAD_POOL_SIZE)));
    
    m_thread_pool_size = optimal_threads;
    m_thread_pool->setMaxThreadCount(static_cast<int>(optimal_threads));
    
    qDebug() << "MultiStreamProcessor: Initialized thread pool with" << optimal_threads << "threads";
}

void MultiStreamProcessor::cleanup_thread_pool() {
    if (m_thread_pool) {
        m_thread_pool->waitForDone(5000);
    }
}

void MultiStreamProcessor::initialize_stream_worker(const QString& stream_id) {
    auto config_it = m_stream_configs.find(stream_id);
    if (config_it == m_stream_configs.end()) {
        return;
    }
    
    // Create worker thread
    auto worker_thread = std::make_unique<QThread>();
    auto worker = std::make_unique<StreamWorker>(config_it.value());
    
    worker->moveToThread(worker_thread.get());
    
    // Connect worker signals
    connect(worker.get(), &StreamWorker::stream_started,
            this, &MultiStreamProcessor::handle_worker_stream_started);
    connect(worker.get(), &StreamWorker::stream_stopped,
            this, &MultiStreamProcessor::handle_worker_stream_stopped);
    connect(worker.get(), &StreamWorker::frame_processed,
            this, &MultiStreamProcessor::handle_worker_frame_processed);
    connect(worker.get(), &StreamWorker::error_occurred,
            this, &MultiStreamProcessor::handle_worker_error);
    connect(worker.get(), &StreamWorker::statistics_updated,
            this, &MultiStreamProcessor::handle_worker_statistics);
    connect(worker.get(), &StreamWorker::quality_threshold_violated,
            this, &MultiStreamProcessor::handle_worker_quality_violation);
    connect(worker.get(), &StreamWorker::failover_triggered,
            this, &MultiStreamProcessor::handle_worker_failover);
    
    // Store worker and thread
    m_workers[stream_id] = std::move(worker);
    m_worker_threads[stream_id] = std::move(worker_thread);
    
    // Start worker thread and stream
    m_worker_threads[stream_id]->start();
    QMetaObject::invokeMethod(m_workers[stream_id].get(), "start_stream", Qt::QueuedConnection);
}

void MultiStreamProcessor::cleanup_stream_worker(const QString& stream_id) {
    auto thread_it = m_worker_threads.find(stream_id);
    if (thread_it != m_worker_threads.end() && thread_it->second) {
        // Stop stream first
        auto worker_it = m_workers.find(stream_id);
        if (worker_it != m_workers.end() && worker_it->second) {
            QMetaObject::invokeMethod(worker_it->second.get(), "stop_stream", Qt::QueuedConnection);
        }
        
        // Stop thread
        thread_it->second->quit();
        if (!thread_it->second->wait(5000)) {
            thread_it->second->terminate();
            thread_it->second->wait(1000);
        }
        m_worker_threads.erase(thread_it);
    }
    
    auto worker_it = m_workers.find(stream_id);
    if (worker_it != m_workers.end()) {
        m_workers.erase(worker_it);
    }
}

void MultiStreamProcessor::validate_stream_config(const StreamConfig& config) {
    if (config.description.isEmpty()) {
        throw std::invalid_argument("Stream description cannot be empty");
    }
    
    if (config.source_type == StreamSourceType::MULTICAST_UDP) {
        if (config.multicast_address.isEmpty()) {
            throw std::invalid_argument("Multicast address cannot be empty");
        }
        
        if (config.port == 0) {
            throw std::invalid_argument("Port cannot be zero");
        }
    }
    
    if (config.buffer_size == 0 || config.buffer_size > 10000) {
        throw std::invalid_argument("Invalid buffer size");
    }
}

QString MultiStreamProcessor::generate_unique_stream_id(const QString& base_id) const {
    QString candidate = base_id.isEmpty() ? "stream" : base_id;
    candidate = candidate.replace(" ", "_").toLower();
    
    int counter = 1;
    QString unique_id = candidate;
    
    while (m_stream_configs.contains(unique_id)) {
        unique_id = QString("%1_%2").arg(candidate).arg(counter++);
    }
    
    return unique_id;
}

bool MultiStreamProcessor::can_add_stream() const {
    return m_stream_configs.size() < static_cast<int>(m_max_concurrent_streams.load());
}

void MultiStreamProcessor::update_global_statistics() {
    QReadLocker locker(&m_streams_lock);
    
    size_t total_frames = 0;
    size_t total_dropped = 0;
    double total_rate = 0.0;
    
    for (auto it = m_stream_status.begin(); it != m_stream_status.end(); ++it) {
        total_frames += it->frames_processed;
        total_dropped += it->frames_dropped;
        total_rate += it->current_frame_rate;
    }
    
    m_total_frames_processed = total_frames;
    m_total_frames_dropped = total_dropped;
    m_total_frame_rate = total_rate;
}

void MultiStreamProcessor::check_resource_usage() {
    // Monitor CPU and memory usage
    monitor_cpu_usage();
    monitor_memory_usage();
    
    // Check if resource thresholds are exceeded
    const double cpu_threshold = 80.0;
    const size_t memory_threshold_mb = 1024; // 1GB
    
    if (m_cpu_usage.load() > cpu_threshold) {
        emit resource_threshold_exceeded("CPU", m_cpu_usage.load());
    }
    
    if (m_memory_usage.load() > memory_threshold_mb * 1024 * 1024) {
        emit resource_threshold_exceeded("Memory", 
                                        static_cast<double>(m_memory_usage.load()) / (1024 * 1024));
    }
}

void MultiStreamProcessor::perform_load_balancing() {
    if (!m_load_balancing_enabled.load()) {
        return;
    }
    
    // Implement load balancing logic based on strategy
    if (m_load_balancing_strategy == "cpu_based") {
        adjust_stream_priorities();
    }
    
    emit load_balancing_adjusted();
}

void MultiStreamProcessor::monitor_cpu_usage() {
    // Simplified CPU monitoring - real implementation would use system APIs
    m_cpu_usage = 25.0; // Placeholder
}

void MultiStreamProcessor::monitor_memory_usage() {
    // Simplified memory monitoring - real implementation would use system APIs
    m_memory_usage = 128 * 1024 * 1024; // 128MB placeholder
}

void MultiStreamProcessor::adjust_stream_priorities() {
    // Adjust stream processing priorities based on performance
    QReadLocker locker(&m_streams_lock);
    
    for (auto it = m_stream_status.begin(); it != m_stream_status.end(); ++it) {
        const auto& status = it.value();
        
        // Lower priority for streams with high error rates
        if (status.frame_loss_rate > 0.05) { // 5% loss
            // Would adjust thread priority here
        }
    }
}

void MultiStreamProcessor::handle_worker_stream_started(const QString& stream_id) {
    emit stream_started(stream_id);
}

void MultiStreamProcessor::handle_worker_stream_stopped(const QString& stream_id) {
    emit stream_stopped(stream_id);
}

void MultiStreamProcessor::handle_worker_frame_processed(const QString& stream_id, const eti::EtiFrame& frame) {
    if (m_frame_callback) {
        m_frame_callback(stream_id, frame);
    }
    
    emit frame_processed(stream_id, frame);
}

void MultiStreamProcessor::handle_worker_error(const QString& stream_id, const QString& error) {
    {
        QMutexLocker locker(&m_errors_mutex);
        if (!m_stream_errors.contains(stream_id)) {
            m_stream_errors[stream_id] = QStringList{};
        }
        
        m_stream_errors[stream_id].append(error);
        
        // Limit error history
        if (m_stream_errors[stream_id].size() > MAX_ERROR_HISTORY) {
            m_stream_errors[stream_id].removeFirst();
        }
    }
    
    if (m_error_callback) {
        m_error_callback(stream_id, error);
    }
    
    emit stream_error_occurred(stream_id, error);
}

void MultiStreamProcessor::handle_worker_statistics(const QString& stream_id, const eti::StreamStatus& status) {
    {
        QWriteLocker locker(&m_streams_lock);
        m_stream_status[stream_id] = status;
    }
    
    if (m_status_callback) {
        m_status_callback(stream_id, status);
    }
    
    emit stream_statistics_updated(stream_id, status);
}

void MultiStreamProcessor::handle_worker_quality_violation(const QString& stream_id, const QString& violation) {
    if (m_quality_callback) {
        m_quality_callback(stream_id, violation);
    }
    
    emit quality_threshold_violated(stream_id, violation);
}

void MultiStreamProcessor::handle_worker_failover(const QString& stream_id, const QString& backup_address) {
    emit failover_triggered(stream_id, backup_address);
}

// MultiStreamProcessorFactory Implementation
std::unique_ptr<MultiStreamProcessor> MultiStreamProcessorFactory::create_monitoring_processor(size_t max_streams) {
    auto processor = std::make_unique<MultiStreamProcessor>();
    
    processor->set_max_concurrent_streams(max_streams);
    processor->enable_load_balancing(true);
    processor->enable_automatic_quality_control(true);
    processor->set_load_balancing_strategy("cpu_based");
    
    // Set conservative quality thresholds for monitoring
    processor->set_global_quality_thresholds(150.0, 0.02, std::chrono::milliseconds{200});
    
    return processor;
}

std::unique_ptr<MultiStreamProcessor> MultiStreamProcessorFactory::create_redundant_processor(size_t max_streams) {
    auto processor = std::make_unique<MultiStreamProcessor>();
    
    processor->set_max_concurrent_streams(max_streams);
    processor->enable_load_balancing(true);
    processor->enable_automatic_quality_control(true);
    processor->enable_global_failover(true);
    processor->set_load_balancing_strategy("priority");
    
    // Set strict quality thresholds for redundancy
    processor->set_global_quality_thresholds(200.0, 0.01, std::chrono::milliseconds{100});
    
    return processor;
}

// MultiStreamUtils Implementation
namespace MultiStreamUtils {

QStringList get_multicast_capable_interfaces() {
    QStringList interfaces;
    
    const auto all_interfaces = QNetworkInterface::allInterfaces();
    for (const auto& interface : all_interfaces) {
        if (interface.flags() & QNetworkInterface::CanMulticast &&
            interface.flags() & QNetworkInterface::IsUp &&
            interface.flags() & QNetworkInterface::IsRunning) {
            interfaces.append(interface.name());
        }
    }
    
    return interfaces;
}

bool is_interface_suitable_for_streaming(const QString& interface_name) {
    const auto interface = QNetworkInterface::interfaceFromName(interface_name);
    
    if (!interface.isValid()) {
        return false;
    }
    
    // Check if interface supports multicast and is active
    return (interface.flags() & QNetworkInterface::CanMulticast) &&
           (interface.flags() & QNetworkInterface::IsUp) &&
           (interface.flags() & QNetworkInterface::IsRunning) &&
           !(interface.flags() & QNetworkInterface::IsLoopBack);
}

MultiStreamUtils::StreamPerformanceReport MultiStreamUtils::generate_performance_report(const StreamStatus& status) {
    StreamPerformanceReport report;
    report.stream_id = status.stream_id;
    report.average_frame_rate = status.average_frame_rate;
    report.frame_loss_rate = status.frame_loss_rate;
    report.average_latency = std::chrono::microseconds{status.average_latency_us};
    report.total_frames = status.frames_processed;
    report.error_count = status.processing_errors;
    
    // Calculate uptime percentage
    const auto now = std::chrono::system_clock::now();
    const auto uptime = std::chrono::duration_cast<std::chrono::seconds>(now - status.start_time);
    const auto total_expected_frames = uptime.count() * 250; // 250 frames per second for ETI
    
    if (total_expected_frames > 0) {
        report.uptime_percentage = static_cast<double>(status.frames_processed) / total_expected_frames * 100.0;
    } else {
        report.uptime_percentage = 0.0;
    }
    
    report.report_time = QDateTime::currentDateTime();
    
    return report;
}

StreamConfig MultiStreamUtils::create_config_from_url(const QString& url) {
    StreamConfig config;
    
    // Parse URL format: "udp://239.192.0.1:9200"
    if (url.startsWith("udp://")) {
        const QString address_port = url.mid(6); // Remove "udp://"
        const QStringList parts = address_port.split(':');
        
        if (parts.size() == 2) {
            config.source_type = StreamSourceType::MULTICAST_UDP;
            config.multicast_address = parts[0];
            config.port = parts[1].toUShort();
            config.description = QString("Stream from %1").arg(url);
        }
    }
    
    return config;
}

} // namespace MultiStreamUtils

} // namespace eti

#include "multi_stream_processor.moc"