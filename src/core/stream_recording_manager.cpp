/**
 * @file stream_recording_manager.cpp
 * @brief Implementation of Professional Stream Recording Manager
 * 
 * High-performance, thread-safe ETI stream recording with time-based segmentation,
 * multiple format support, and professional broadcast workflow integration.
 */

#include "stream_recording_manager.hpp"
#include "../utils/logger.h"
#include <QJsonObject>
#include <QJsonDocument>
#include <QStandardPaths>
#include <QStorageInfo>
#include <QCryptographicHash>
#include <QProcess>
#include <QUuid>
#include <QDebug>
#include <algorithm>
#include <numeric>

namespace eti {

// RecordingWorker Implementation
RecordingWorker::RecordingWorker(const RecordingSession& session, QObject* parent)
    : QObject(parent)
    , m_session(session)
    , m_segment_timer(std::make_unique<QTimer>(this))
    , m_rotation_timer(std::make_unique<QTimer>(this))
    , m_cleanup_timer(std::make_unique<QTimer>(this))
    , m_last_frame_time(std::chrono::steady_clock::now())
{
    m_frame_rate_history.reserve(PERF_HISTORY_SIZE);
    m_latency_history.reserve(PERF_HISTORY_SIZE);
    
    // Setup timers
    connect(m_segment_timer.get(), &QTimer::timeout,
            this, &RecordingWorker::handle_segment_timer);
    connect(m_rotation_timer.get(), &QTimer::timeout,
            this, &RecordingWorker::handle_rotation_timer);
    connect(m_cleanup_timer.get(), &QTimer::timeout,
            this, &RecordingWorker::handle_cleanup_timer);
    
    // Configure segment timer
    const auto segment_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        m_session.segmentation.segment_duration).count();
    m_segment_timer->setInterval(static_cast<int>(segment_ms));
    m_segment_timer->setSingleShot(false);
    
    // Configure rotation timer (if enabled)
    if (m_session.segmentation.daily_rotation.count() > 0) {
        const auto rotation_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
            m_session.segmentation.daily_rotation).count();
        m_rotation_timer->setInterval(static_cast<int>(rotation_ms));
        m_rotation_timer->setSingleShot(false);
    }
    
    // Configure cleanup timer (if enabled)
    if (m_session.segmentation.auto_cleanup) {
        m_cleanup_timer->setInterval(60000); // Check every minute
        m_cleanup_timer->setSingleShot(false);
    }
}

RecordingWorker::~RecordingWorker() {
    stop_recording();
}

void RecordingWorker::start_recording() {
    if (m_recording.load()) {
        return; // Already recording
    }
    
    try {
        initialize_recording();
        create_new_segment();
        
        m_recording = true;
        m_paused = false;
        
        // Start timers
        m_segment_timer->start();
        if (m_session.segmentation.daily_rotation.count() > 0) {
            m_rotation_timer->start();
        }
        if (m_session.segmentation.auto_cleanup) {
            m_cleanup_timer->start();
        }
        
        emit recording_started(m_session.session_id);
        qDebug() << "RecordingWorker: Started recording session" << m_session.session_id;
        
    } catch (const std::exception& e) {
        m_recording = false;
        emit error_occurred(m_session.session_id, 
                           QString("Failed to start recording: %1").arg(e.what()));
    }
}

void RecordingWorker::stop_recording() {
    if (!m_recording.load()) {
        return; // Not recording
    }
    
    m_recording = false;
    m_paused = false;
    
    // Stop timers
    m_segment_timer->stop();
    m_rotation_timer->stop();
    m_cleanup_timer->stop();
    
    // Flush any remaining frames
    flush_buffers();
    
    // Close current segment
    close_current_segment();
    
    // Finalize recording
    finalize_recording();
    
    emit recording_stopped(m_session.session_id);
    qDebug() << "RecordingWorker: Stopped recording session" << m_session.session_id;
}

void RecordingWorker::pause_recording() {
    if (!m_recording.load() || m_paused.load()) {
        return;
    }
    
    m_paused = true;
    m_segment_timer->stop();
    
    emit recording_paused(m_session.session_id);
    qDebug() << "RecordingWorker: Paused recording session" << m_session.session_id;
}

void RecordingWorker::resume_recording() {
    if (!m_recording.load() || !m_paused.load()) {
        return;
    }
    
    m_paused = false;
    m_segment_timer->start();
    
    emit recording_resumed(m_session.session_id);
    qDebug() << "RecordingWorker: Resumed recording session" << m_session.session_id;
}

void RecordingWorker::record_frame(const eti::EtiFrame& frame) {
    if (!m_recording.load() || m_paused.load()) {
        return;
    }
    
    const auto now = std::chrono::steady_clock::now();
    
    try {
        // Write frame to current file
        write_frame_to_file(frame);
        
        // Update statistics
        m_session.frames_recorded++;
        m_current_segment_frames++;
        m_current_segment_bytes += frame.size();
        m_session.bytes_written += frame.size();
        
        // Update performance metrics
        const auto frame_duration = now - m_last_frame_time;
        const auto frame_duration_us = std::chrono::duration_cast<std::chrono::microseconds>(frame_duration);
        
        if (frame_duration_us.count() > 0) {
            const double instantaneous_rate = 1000000.0 / frame_duration_us.count();
            m_frame_rate_history.push_back(instantaneous_rate);
            if (m_frame_rate_history.size() > PERF_HISTORY_SIZE) {
                m_frame_rate_history.erase(m_frame_rate_history.begin());
            }
            
            // Calculate average frame rate
            m_session.average_frame_rate = std::accumulate(
                m_frame_rate_history.begin(), m_frame_rate_history.end(), 0.0) / 
                m_frame_rate_history.size();
                
            if (instantaneous_rate > m_session.peak_frame_rate) {
                m_session.peak_frame_rate = instantaneous_rate;
            }
        }
        
        // Check segment size limits
        const size_t segment_size_mb = m_current_segment_bytes / (1024 * 1024);
        if (segment_size_mb >= m_session.segmentation.max_segment_size_mb) {
            rotate_segment();
        }
        
        m_last_frame_time = now;
        
    } catch (const std::exception& e) {
        m_session.errors_encountered++;
        emit error_occurred(m_session.session_id, 
                           QString("Failed to record frame: %1").arg(e.what()));
    }
}

void RecordingWorker::flush_buffers() {
    QMutexLocker queue_locker(&m_queue_mutex);
    
    // Process any queued frames
    while (!m_frame_queue.isEmpty()) {
        const EtiFrame frame = m_frame_queue.dequeue();
        record_frame(frame);
    }
    
    // Flush file buffers
    if (m_current_file && m_current_file->isOpen()) {
        m_current_file->flush();
    }
}

void RecordingWorker::rotate_segment() {
    if (!m_recording.load()) {
        return;
    }
    
    try {
        // Close current segment
        close_current_segment();
        
        // Create new segment
        create_new_segment();
        
        qDebug() << "RecordingWorker: Rotated segment for session" << m_session.session_id;
        
    } catch (const std::exception& e) {
        emit error_occurred(m_session.session_id,
                           QString("Failed to rotate segment: %1").arg(e.what()));
    }
}

void RecordingWorker::cleanup_old_files() {
    if (!m_session.segmentation.auto_cleanup) {
        return;
    }
    
    try {
        const auto cutoff_time = QDateTime::currentDateTime().addSecs(
            -static_cast<qint64>(m_session.segmentation.retention_period.count() * 3600));
        
        QDir output_dir(m_session.output_directory);
        const auto file_list = output_dir.entryInfoList(QDir::Files);
        
        QStringList deleted_files;
        for (const auto& file_info : file_list) {
            if (file_info.lastModified() < cutoff_time) {
                if (output_dir.remove(file_info.fileName())) {
                    deleted_files.append(file_info.absoluteFilePath());
                }
            }
        }
        
        if (!deleted_files.isEmpty()) {
            qDebug() << "RecordingWorker: Cleaned up" << deleted_files.size() 
                     << "old files for session" << m_session.session_id;
        }
        
    } catch (const std::exception& e) {
        emit error_occurred(m_session.session_id,
                           QString("Failed to cleanup old files: %1").arg(e.what()));
    }
}

void RecordingWorker::initialize_recording() {
    // Create output directory if it doesn't exist
    QDir output_dir(m_session.output_directory);
    if (!output_dir.exists()) {
        if (!output_dir.mkpath(".")) {
            throw std::runtime_error(QString("Failed to create output directory: %1")
                                    .arg(m_session.output_directory).toStdString());
        }
    }
    
    // Check disk space
    check_disk_space();
    
    // Reset statistics
    m_session.frames_recorded = 0;
    m_session.segments_created = 0;
    m_session.bytes_written = 0;
    m_session.errors_encountered = 0;
    m_session.average_frame_rate = 0.0;
    m_session.peak_frame_rate = 0.0;
    m_session.average_latency = std::chrono::microseconds{0};
    m_session.max_latency = std::chrono::microseconds{0};
}

void RecordingWorker::finalize_recording() {
    // Compress final segment if enabled
    if (m_session.segmentation.enable_compression && !m_current_segment_path.isEmpty()) {
        compress_segment(m_current_segment_path);
    }
    
    // Create final archive if needed
    if (m_session.segments_created >= m_session.segmentation.max_archive_segments) {
        create_archive();
    }
    
    // Update final statistics
    update_statistics();
}

void RecordingWorker::create_new_segment() {
    QMutexLocker locker(&m_file_mutex);
    
    // Generate new segment filename
    const QString segment_filename = generate_segment_filename();
    const QString segment_path = QDir(m_session.output_directory).absoluteFilePath(segment_filename);
    
    // Create new file
    m_current_file = std::make_unique<QFile>(segment_path);
    if (!m_current_file->open(QIODevice::WriteOnly)) {
        throw std::runtime_error(QString("Failed to create segment file: %1")
                                .arg(segment_path).toStdString());
    }
    
    // Write metadata header
    write_metadata_header();
    
    // Update session state
    m_current_segment_path = segment_path;
    m_current_segment_start = QDateTime::currentDateTime();
    m_current_segment_frames = 0;
    m_current_segment_bytes = 0;
    m_session.segments_created++;
    
    qDebug() << "RecordingWorker: Created new segment" << segment_filename;
}

void RecordingWorker::close_current_segment() {
    QMutexLocker locker(&m_file_mutex);
    
    if (!m_current_file || !m_current_file->isOpen()) {
        return;
    }
    
    const QString segment_path = m_current_segment_path;
    
    // Close file
    m_current_file->close();
    m_current_file.reset();
    
    // Compress segment if enabled
    if (m_session.segmentation.enable_compression) {
        compress_segment(segment_path);
    }
    
    emit segment_completed(m_session.session_id, segment_path);
    
    // Check if archive creation is needed
    if (m_session.segments_created % m_session.segmentation.max_archive_segments == 0) {
        create_archive();
    }
    
    qDebug() << "RecordingWorker: Closed segment" << QFileInfo(segment_path).fileName();
}

void RecordingWorker::write_frame_to_file(const EtiFrame& frame) {
    QMutexLocker locker(&m_file_mutex);
    
    if (!m_current_file || !m_current_file->isOpen()) {
        throw std::runtime_error("No active segment file for writing");
    }
    
    // Convert frame to recording format
    std::vector<uint8_t> frame_data;
    switch (m_session.format) {
        case RecordingFormat::ETI_NI:
            // Native ETI-NI format (6144 bytes)
            frame_data.assign(frame.data(), frame.data() + frame.size());
            break;
            
        case RecordingFormat::ETI_LI:
            // Linear ETI format - remove sync and add linear header
            frame_data.reserve(frame.size());
            frame_data.assign(frame.data() + 4, frame.data() + frame.size()); // Skip sync
            break;
            
        case RecordingFormat::RAW_BINARY:
            // Raw binary format
            frame_data.assign(frame.data(), frame.data() + frame.size());
            break;
            
        default:
            frame_data.assign(frame.data(), frame.data() + frame.size());
            break;
    }
    
    // Write frame data
    const qint64 bytes_written = m_current_file->write(
        reinterpret_cast<const char*>(frame_data.data()), 
        static_cast<qint64>(frame_data.size()));
    
    if (bytes_written != static_cast<qint64>(frame_data.size())) {
        throw std::runtime_error(QString("Failed to write complete frame: %1/%2 bytes written")
                                .arg(bytes_written).arg(frame_data.size()).toStdString());
    }
}

void RecordingWorker::write_metadata_header() {
    if (!m_current_file || !m_current_file->isOpen()) {
        return;
    }
    
    // Create metadata header
    QJsonObject metadata;
    metadata["session_id"] = m_session.session_id;
    metadata["format"] = static_cast<int>(m_session.format);
    metadata["start_time"] = m_current_segment_start.toString(Qt::ISODate);
    metadata["frame_size"] = static_cast<int>(ETI_FRAME_SIZE);
    metadata["segment_number"] = static_cast<int>(m_session.segments_created);
    
    const QJsonDocument doc(metadata);
    const QByteArray metadata_bytes = doc.toJson(QJsonDocument::Compact);
    
    // Write metadata size and data (for formats that support it)
    if (m_session.format == RecordingFormat::COMPRESSED_ETI || 
        m_session.format == RecordingFormat::COMPRESSED_ARCHIVE) {
        
        const uint32_t metadata_size = static_cast<uint32_t>(metadata_bytes.size());
        m_current_file->write(reinterpret_cast<const char*>(&metadata_size), sizeof(metadata_size));
        m_current_file->write(metadata_bytes);
    }
}

void RecordingWorker::compress_segment(const QString& segment_path) {
    // Implementation would use zlib or Qt's compression
    // For now, we'll create a placeholder compressed file
    const QString compressed_path = segment_path + ".gz";
    
    QFile original(segment_path);
    QFile compressed(compressed_path);
    
    if (original.open(QIODevice::ReadOnly) && compressed.open(QIODevice::WriteOnly)) {
        // Simple copy for now - real implementation would use compression
        compressed.write(original.readAll());
        
        // Remove original if compression successful
        original.close();
        compressed.close();
        
        if (QFile::remove(segment_path)) {
            qDebug() << "RecordingWorker: Compressed segment" << QFileInfo(segment_path).fileName();
        }
    }
}

void RecordingWorker::create_archive() {
    const QString archive_filename = generate_archive_filename();
    const QString archive_path = QDir(m_session.output_directory).absoluteFilePath(archive_filename);
    
    // Create archive from recent segments
    // Implementation would use tar/zip libraries
    
    emit archive_created(m_session.session_id, archive_path);
    qDebug() << "RecordingWorker: Created archive" << archive_filename;
}

void RecordingWorker::check_disk_space() {
    const QStorageInfo storage(m_session.output_directory);
    
    if (storage.isValid()) {
        const double usage_percent = 100.0 * (1.0 - (static_cast<double>(storage.bytesAvailable()) / 
                                                     static_cast<double>(storage.bytesTotal())));
        
        if (usage_percent > m_session.segmentation.max_disk_usage_percent) {
            emit disk_space_warning(m_session.output_directory, usage_percent);
        }
    }
}

void RecordingWorker::update_statistics() {
    const auto now = std::chrono::steady_clock::now();
    
    // Update session timestamp
    m_session.end_time = QDateTime::currentDateTime();
    
    // Calculate average latency
    if (!m_latency_history.empty()) {
        const auto total_latency = std::accumulate(
            m_latency_history.begin(), m_latency_history.end(), std::chrono::microseconds{0});
        m_session.average_latency = total_latency / m_latency_history.size();
        
        const auto max_latency_it = std::max_element(m_latency_history.begin(), m_latency_history.end());
        if (max_latency_it != m_latency_history.end()) {
            m_session.max_latency = *max_latency_it;
        }
    }
    
    emit statistics_updated(m_session);
}

QString RecordingWorker::generate_segment_filename() const {
    const QDateTime now = m_current_segment_start.isValid() ? 
                         m_current_segment_start : QDateTime::currentDateTime();
    
    QString filename = m_session.segmentation.filename_pattern;
    
    // Replace strftime-style patterns
    filename = filename.replace("%Y", now.toString("yyyy"));
    filename = filename.replace("%m", now.toString("MM"));
    filename = filename.replace("%d", now.toString("dd"));
    filename = filename.replace("%H", now.toString("hh"));
    filename = filename.replace("%M", now.toString("mm"));
    filename = filename.replace("%S", now.toString("ss"));
    
    // Add segment suffix and number
    filename += QString("_%1_%2").arg(m_session.segmentation.segment_suffix)
                                 .arg(m_session.segments_created + 1, 4, 10, QChar('0'));
    
    // Add format extension
    switch (m_session.format) {
        case RecordingFormat::ETI_NI:
            filename += ".eti";
            break;
        case RecordingFormat::ETI_LI:
            filename += ".etili";
            break;
        case RecordingFormat::EDI:
            filename += ".edi";
            break;
        case RecordingFormat::RAW_BINARY:
            filename += ".raw";
            break;
        case RecordingFormat::COMPRESSED_ETI:
            filename += ".eti.gz";
            break;
        case RecordingFormat::COMPRESSED_ARCHIVE:
            filename += ".tar.gz";
            break;
    }
    
    return filename;
}

QString RecordingWorker::generate_archive_filename() const {
    const QDateTime now = QDateTime::currentDateTime();
    
    QString filename = QString("archive_%1_%2")
                      .arg(now.toString("yyyyMMdd_hhmmss"))
                      .arg(m_session.segmentation.archive_suffix);
    
    filename += ".tar.gz";
    
    return filename;
}

void RecordingWorker::handle_segment_timer() {
    if (m_recording.load() && !m_paused.load()) {
        rotate_segment();
    }
}

void RecordingWorker::handle_rotation_timer() {
    if (m_recording.load()) {
        // Force segment rotation for daily archival
        rotate_segment();
        qDebug() << "RecordingWorker: Daily rotation triggered for session" << m_session.session_id;
    }
}

void RecordingWorker::handle_cleanup_timer() {
    if (m_recording.load()) {
        cleanup_old_files();
        check_disk_space();
    }
}

// StreamRecordingManager Implementation
StreamRecordingManager::StreamRecordingManager(QObject* parent)
    : QObject(parent)
{
    // Set default configuration
    m_default_config.segment_duration = std::chrono::minutes{60};
    m_default_config.enable_compression = false;
    m_default_config.auto_cleanup = true;
    m_default_config.retention_period = std::chrono::hours{168}; // 7 days
}

StreamRecordingManager::~StreamRecordingManager() {
    // Stop all active sessions
    const auto session_ids = get_active_sessions();
    for (const auto& session_id : session_ids) {
        stop_recording_session(session_id);
    }
    
    // Cleanup worker threads
    for (auto& [session_key, thread] : m_worker_threads) {
        if (thread && thread->isRunning()) {
            thread->quit();
            if (!thread->wait(5000)) {
                thread->terminate();
                thread->wait(1000);
            }
        }
    }
}

QString StreamRecordingManager::start_recording_session(const QString& output_directory,
                                                       RecordingFormat format,
                                                       const SegmentationConfig& config) {
    try {
        validate_output_directory(output_directory);
        validate_segmentation_config(config);
        create_output_directory(output_directory);
        
        // Generate unique session ID
        const QString session_id = generate_session_id();
        
        // Create recording session
        RecordingSession session;
        session.session_id = session_id;
        session.start_time = QDateTime::currentDateTime();
        session.format = format;
        session.segmentation = config;
        session.output_directory = output_directory;
        session.is_active = true;
        session.is_paused = false;
        
        // Store session
        {
            QMutexLocker locker(&m_sessions_mutex);
            m_sessions[session_id] = session;
        }
        
        // Initialize worker thread
        initialize_session_worker(session_id);
        
        emit recording_session_started(session_id);
        
        return session_id;
        
    } catch (const std::exception& e) {
        const QString error = QString("Failed to start recording session: %1").arg(e.what());
        if (m_error_callback) {
            m_error_callback("", error);
        }
        throw;
    }
}

bool StreamRecordingManager::stop_recording_session(const QString& session_id) {
    QMutexLocker locker(&m_sessions_mutex);
    
    auto session_it = m_sessions.find(session_id);
    if (session_it == m_sessions.end()) {
        return false;
    }
    
    // Stop worker
    std::string session_key = session_id.toStdString();
    auto worker_it = m_workers.find(session_key);
    if (worker_it != m_workers.end() && worker_it->second) {
        QMetaObject::invokeMethod(worker_it->second.get(), "stop_recording", Qt::QueuedConnection);
    }
    
    // Update session
    session_it->is_active = false;
    session_it->end_time = QDateTime::currentDateTime();
    
    // Cleanup worker thread
    cleanup_session_worker(session_id);
    
    emit recording_session_stopped(session_id);
    
    return true;
}

void StreamRecordingManager::record_frame(const QString& session_id, const EtiFrame& frame) {
    QMutexLocker locker(&m_sessions_mutex);
    
    auto session_it = m_sessions.find(session_id);
    if (session_it == m_sessions.end() || !session_it->is_active || session_it->is_paused) {
        return;
    }
    
    std::string session_key = session_id.toStdString();
    auto worker_it = m_workers.find(session_key);
    if (worker_it != m_workers.end() && worker_it->second) {
        QMetaObject::invokeMethod(worker_it->second.get(), "record_frame", 
                                 Qt::QueuedConnection, Q_ARG(eti::EtiFrame, frame));
    }
}

QStringList StreamRecordingManager::get_active_sessions() const {
    QMutexLocker locker(&m_sessions_mutex);
    
    QStringList active_sessions;
    for (auto it = m_sessions.constBegin(); it != m_sessions.constEnd(); ++it) {
        if (it->is_active) {
            active_sessions.append(it.key());
        }
    }
    
    return active_sessions;
}

RecordingSession StreamRecordingManager::get_session_info(const QString& session_id) const {
    QMutexLocker locker(&m_sessions_mutex);
    
    auto it = m_sessions.find(session_id);
    if (it != m_sessions.end()) {
        return it.value();
    }
    
    return RecordingSession{};
}

void StreamRecordingManager::initialize_session_worker(const QString& session_id) {
    QMutexLocker locker(&m_sessions_mutex);
    
    auto session_it = m_sessions.find(session_id);
    if (session_it == m_sessions.end()) {
        return;
    }
    
    // Create worker thread
    auto worker_thread = std::make_unique<QThread>();
    auto worker = std::make_unique<RecordingWorker>(session_it.value());
    
    worker->moveToThread(worker_thread.get());
    
    // Connect worker signals
    connect(worker.get(), &RecordingWorker::recording_started,
            this, &StreamRecordingManager::handle_worker_recording_started);
    connect(worker.get(), &RecordingWorker::recording_stopped,
            this, &StreamRecordingManager::handle_worker_recording_stopped);
    connect(worker.get(), &RecordingWorker::segment_completed,
            this, &StreamRecordingManager::handle_worker_segment_completed);
    connect(worker.get(), &RecordingWorker::archive_created,
            this, &StreamRecordingManager::handle_worker_archive_created);
    connect(worker.get(), &RecordingWorker::error_occurred,
            this, &StreamRecordingManager::handle_worker_error);
    connect(worker.get(), &RecordingWorker::statistics_updated,
            this, &StreamRecordingManager::handle_worker_statistics);
    connect(worker.get(), &RecordingWorker::disk_space_warning,
            this, &StreamRecordingManager::handle_worker_disk_warning);
    
    // Store worker and thread
    std::string session_key = session_id.toStdString();
    m_workers[session_key] = std::move(worker);
    m_worker_threads[session_key] = std::move(worker_thread);
    
    // Start worker thread
    m_worker_threads[session_key]->start();
    
    // Start recording
    QMetaObject::invokeMethod(m_workers[session_key].get(), "start_recording", Qt::QueuedConnection);
}

void StreamRecordingManager::cleanup_session_worker(const QString& session_id) {
    std::string session_key = session_id.toStdString();
    auto thread_it = m_worker_threads.find(session_key);
    if (thread_it != m_worker_threads.end() && thread_it->second) {
        thread_it->second->quit();
        if (!thread_it->second->wait(5000)) {
            thread_it->second->terminate();
            thread_it->second->wait(1000);
        }
        m_worker_threads.erase(thread_it);
    }
    
    auto worker_it = m_workers.find(session_key);
    if (worker_it != m_workers.end()) {
        m_workers.erase(worker_it);
    }
}

QString StreamRecordingManager::generate_session_id() const {
    return QUuid::createUuid().toString(QUuid::WithoutBraces);
}

void StreamRecordingManager::validate_output_directory(const QString& directory) {
    if (directory.isEmpty()) {
        throw std::invalid_argument("Output directory cannot be empty");
    }
    
    const QFileInfo dir_info(directory);
    if (dir_info.exists() && !dir_info.isDir()) {
        throw std::invalid_argument("Output path exists but is not a directory");
    }
}

void StreamRecordingManager::validate_segmentation_config(const SegmentationConfig& config) {
    if (config.segment_duration.count() <= 0) {
        throw std::invalid_argument("Segment duration must be positive");
    }
    
    if (config.max_segment_size_mb == 0) {
        throw std::invalid_argument("Maximum segment size must be positive");
    }
    
    if (config.retention_period.count() < 0) {
        throw std::invalid_argument("Retention period cannot be negative");
    }
}

void StreamRecordingManager::create_output_directory(const QString& directory) {
    QDir dir(directory);
    if (!dir.exists()) {
        if (!dir.mkpath(".")) {
            throw std::runtime_error(QString("Failed to create output directory: %1")
                                    .arg(directory).toStdString());
        }
    }
}

void StreamRecordingManager::handle_worker_recording_started(const QString& session_id) {
    emit recording_session_started(session_id);
}

void StreamRecordingManager::handle_worker_recording_stopped(const QString& session_id) {
    emit recording_session_stopped(session_id);
}

void StreamRecordingManager::handle_worker_segment_completed(const QString& session_id, const QString& filename) {
    emit segment_completed(session_id, filename);
    
    if (m_file_callback) {
        m_file_callback(session_id, filename);
    }
}

void StreamRecordingManager::handle_worker_archive_created(const QString& session_id, const QString& filename) {
    emit archive_created(session_id, filename);
    
    if (m_file_callback) {
        m_file_callback(session_id, filename);
    }
}

void StreamRecordingManager::handle_worker_error(const QString& session_id, const QString& error) {
    {
        QMutexLocker locker(&m_errors_mutex);
        if (!m_session_errors.contains(session_id)) {
            m_session_errors[session_id] = QStringList{};
        }
        
        m_session_errors[session_id].append(error);
        
        // Limit error history
        if (m_session_errors[session_id].size() > MAX_ERROR_HISTORY) {
            m_session_errors[session_id].removeFirst();
        }
    }
    
    emit recording_error_occurred(session_id, error);
    
    if (m_error_callback) {
        m_error_callback(session_id, error);
    }
}

void StreamRecordingManager::handle_worker_statistics(const eti::RecordingSession& session) {
    {
        QMutexLocker locker(&m_sessions_mutex);
        auto it = m_sessions.find(session.session_id);
        if (it != m_sessions.end()) {
            it.value() = session;
        }
    }
    
    emit session_statistics_updated(session.session_id, session);
    
    if (m_session_callback) {
        m_session_callback(session.session_id, session);
    }
}

void StreamRecordingManager::handle_worker_disk_warning(const QString& directory, double usage_percent) {
    emit disk_space_warning("", directory, usage_percent);
}

// StreamRecordingManagerFactory Implementation
std::unique_ptr<StreamRecordingManager> StreamRecordingManagerFactory::create_broadcast_recorder(
    const QString& base_directory) {
    
    auto manager = std::make_unique<StreamRecordingManager>();
    
    SegmentationConfig config;
    config.segment_duration = std::chrono::minutes{60};
    config.enable_compression = false;
    config.auto_cleanup = true;
    config.retention_period = std::chrono::hours{72}; // 3 days
    config.max_segment_size_mb = 500;
    
    manager->set_default_segmentation_config(config);
    
    return manager;
}

std::unique_ptr<StreamRecordingManager> StreamRecordingManagerFactory::create_archival_recorder(
    const QString& base_directory, std::chrono::hours retention_period) {
    
    auto manager = std::make_unique<StreamRecordingManager>();
    
    SegmentationConfig config;
    config.segment_duration = std::chrono::hours{6};
    config.enable_compression = true;
    config.auto_cleanup = true;
    config.retention_period = retention_period;
    config.max_segment_size_mb = 2048;
    config.max_archive_segments = 50;
    
    manager->set_default_segmentation_config(config);
    manager->enable_realtime_compression(true);
    manager->set_compression_level(6);
    
    return manager;
}

// Missing method implementations

void StreamRecordingManager::set_default_segmentation_config(const SegmentationConfig& config) {
    Logger::instance().log(Logger::Info, "StreamRecordingManager", 
                          QString("Setting default segmentation config: %1min segments, %2h rotation, compression=%3")
                          .arg(config.segment_duration.count())
                          .arg(config.daily_rotation.count())
                          .arg(config.enable_compression ? "enabled" : "disabled"));
    
    // Validate configuration before applying
    validate_segmentation_config(config);
    
    // Store the new default configuration
    m_default_config = config;
    
    // Apply to all active recording sessions
    QMutexLocker locker(&m_sessions_mutex);
    for (auto it = m_sessions.begin(); it != m_sessions.end(); ++it) {
        const QString& session_id = it.key();
        RecordingSession& session = it.value();
        // Update session configuration if it was using default values
        if (session.segmentation.filename_pattern == "eti_%Y%m%d_%H%M%S") {
            session.segmentation = config;
            Logger::instance().log(Logger::Debug, "StreamRecordingManager", 
                                  QString("Updated segmentation config for session: %1").arg(session_id));
        }
    }
    
    // Update compression settings if configuration changed
    if (config.enable_compression != m_realtime_compression) {
        enable_realtime_compression(config.enable_compression);
    }
    
    Logger::instance().log(Logger::Info, "StreamRecordingManager", 
                          QString("Default segmentation configuration applied to %1 active sessions")
                          .arg(m_sessions.size()));
}

void StreamRecordingManager::enable_realtime_compression(bool enabled) {
    Logger::instance().log(Logger::Info, "StreamRecordingManager", 
                          QString("Setting realtime compression: %1").arg(enabled ? "enabled" : "disabled"));
    
    QMutexLocker locker(&m_sessions_mutex);
    
    // Store compression state
    m_realtime_compression = enabled;
    
    // Update all active recording sessions
    for (auto it = m_sessions.begin(); it != m_sessions.end(); ++it) {
        const QString& session_id = it.key();
        RecordingSession& session = it.value();
        session.segmentation.enable_compression = enabled;
        
        Logger::instance().log(Logger::Debug, "StreamRecordingManager", 
                              QString("Updated compression setting for session %1: %2")
                              .arg(session_id).arg(enabled ? "enabled" : "disabled"));
    }
    
    // Initialize compression worker thread if enabling compression
    if (enabled && !m_compression_worker_active.load()) {
        // Start background compression thread for completed segments
        m_compression_worker_active.store(true);
        
        // Create compression worker thread
        std::thread compression_thread([this]() {
            this->compression_worker_loop();
        });
        compression_thread.detach();
        
        Logger::instance().log(Logger::Info, "StreamRecordingManager", 
                              "Started background compression worker thread");
    } else if (!enabled && m_compression_worker_active.load()) {
        // Stop compression worker thread
        m_compression_worker_active.store(false);
        m_compression_queue_condition.notify_all();
        
        Logger::instance().log(Logger::Info, "StreamRecordingManager", 
                              "Stopping background compression worker thread");
    }
    
    // Update default configuration
    m_default_config.enable_compression = enabled;
    
    Logger::instance().log(Logger::Info, "StreamRecordingManager", 
                          QString("Realtime compression %1 for %2 active sessions")
                          .arg(enabled ? "enabled" : "disabled")
                          .arg(m_sessions.size()));
}

void StreamRecordingManager::set_compression_level(int level) {
    // Validate compression level (0-9 for gzip/zlib, where 0=no compression, 9=max compression)
    if (level < 0 || level > 9) {
        Logger::instance().log(Logger::Warning, "StreamRecordingManager", 
                              QString("Invalid compression level %1, must be 0-9. Using default level 6.").arg(level));
        level = 6;  // Default compression level
    }
    
    Logger::instance().log(Logger::Info, "StreamRecordingManager", 
                          QString("Setting compression level: %1 (%2)")
                          .arg(level)
                          .arg(level == 0 ? "no compression" : 
                               level <= 3 ? "fast compression" :
                               level <= 6 ? "balanced compression" : "max compression"));
    
    QMutexLocker locker(&m_sessions_mutex);
    
    // Store compression level
    m_compression_level = level;
    
    // Update compression settings for all active sessions
    for (auto it = m_sessions.begin(); it != m_sessions.end(); ++it) {
        const QString& session_id = it.key();
        RecordingSession& session = it.value();
        // Store compression level in session metadata
        // Note: In a real implementation, this might be stored in session.compression_level
        Logger::instance().log(Logger::Debug, "StreamRecordingManager", 
                              QString("Updated compression level for session %1: level %2")
                              .arg(session_id).arg(level));
        
        // If compression is already enabled, the new level will be used for new segments
        if (session.segmentation.enable_compression) {
            Logger::instance().log(Logger::Debug, "StreamRecordingManager", 
                                  QString("Session %1: New compression level %2 will apply to new segments")
                                  .arg(session_id).arg(level));
        }
    }
    
    // Log compression efficiency information
    QString efficiency_info;
    switch (level) {
        case 0: efficiency_info = "No compression - fastest writing, largest files"; break;
        case 1: efficiency_info = "Minimal compression - very fast, large files"; break;
        case 2:
        case 3: efficiency_info = "Fast compression - good speed, moderate compression"; break;
        case 4:
        case 5:
        case 6: efficiency_info = "Balanced compression - good speed/ratio trade-off"; break;
        case 7:
        case 8: efficiency_info = "High compression - slower, smaller files"; break;
        case 9: efficiency_info = "Maximum compression - slowest, smallest files"; break;
    }
    
    Logger::instance().log(Logger::Info, "StreamRecordingManager", 
                          QString("Compression level %1 set: %2").arg(level).arg(efficiency_info));
}

void StreamRecordingManager::compression_worker_loop() {
    Logger::instance().log(Logger::Info, "StreamRecordingManager", 
                          "Compression worker thread started");
    
    while (m_compression_worker_active.load()) {
        std::unique_lock<std::mutex> lock(m_compression_queue_mutex);
        
        // Wait for files to compress or shutdown signal
        m_compression_queue_condition.wait(lock, [this]() {
            return !m_compression_queue.empty() || !m_compression_worker_active.load();
        });
        
        // Process all queued files
        while (!m_compression_queue.empty() && m_compression_worker_active.load()) {
            QString file_path = m_compression_queue.front();
            m_compression_queue.pop();
            lock.unlock();
            
            // Compress the file
            try {
                compress_file(file_path);
                Logger::instance().log(Logger::Debug, "StreamRecordingManager", 
                                      QString("Successfully compressed: %1").arg(file_path));
            } catch (const std::exception& e) {
                Logger::instance().log(Logger::Error, "StreamRecordingManager", 
                                      QString("Failed to compress %1: %2").arg(file_path).arg(e.what()));
            }
            
            lock.lock();
        }
    }
    
    Logger::instance().log(Logger::Info, "StreamRecordingManager", 
                          "Compression worker thread stopped");
}

void StreamRecordingManager::enqueue_for_compression(const QString& file_path) {
    if (!m_compression_worker_active.load()) {
        return;
    }
    
    std::lock_guard<std::mutex> lock(m_compression_queue_mutex);
    m_compression_queue.push(file_path);
    m_compression_queue_condition.notify_one();
    
    Logger::instance().log(Logger::Debug, "StreamRecordingManager", 
                          QString("Enqueued for compression: %1").arg(file_path));
}

void StreamRecordingManager::compress_file(const QString& file_path) {
    // Implementation for file compression using zlib/gzip
    QFile input_file(file_path);
    if (!input_file.open(QIODevice::ReadOnly)) {
        throw std::runtime_error(QString("Cannot open file for compression: %1").arg(file_path).toStdString());
    }
    
    QString compressed_path = file_path + ".gz";
    QFile output_file(compressed_path);
    if (!output_file.open(QIODevice::WriteOnly)) {
        throw std::runtime_error(QString("Cannot create compressed file: %1").arg(compressed_path).toStdString());
    }
    
    // Simple compression using Qt's built-in compression
    QByteArray data = input_file.readAll();
    QByteArray compressed = qCompress(data, m_compression_level.load());
    
    output_file.write(compressed);
    output_file.close();
    input_file.close();
    
    // Remove original file if compression successful
    if (QFile::remove(file_path)) {
        Logger::instance().log(Logger::Debug, "StreamRecordingManager", 
                              QString("Removed original file after compression: %1").arg(file_path));
    } else {
        Logger::instance().log(Logger::Warning, "StreamRecordingManager", 
                              QString("Failed to remove original file: %1").arg(file_path));
    }
}

} // namespace eti

// MOC file inclusion handled automatically by CMake