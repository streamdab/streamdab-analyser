#include "multi_stream_processor.hpp"
#include <QFile>
#include <QFileInfo>
#include <QUuid>
#include <QDebug>
#include <QStandardPaths>
#include <algorithm>
#include <numeric>

namespace multistream {

MultiStreamProcessor::MultiStreamProcessor(QObject* parent)
    : QObject(parent)
    , m_processing_active(false)
    , m_max_parallel_streams(4)
    , m_processing_thread_count(4)
    , m_comparative_analysis_enabled(true)
{
    // Initialize processing timer for monitoring
    m_processing_timer = std::make_unique<QTimer>();
    m_processing_timer->setInterval(1000); // Update every second
    connect(m_processing_timer.get(), &QTimer::timeout, this, &MultiStreamProcessor::onProcessingTimerTimeout);
}

MultiStreamProcessor::~MultiStreamProcessor()
{
    stopParallelProcessing();
    clearAllStreams();
}

QString MultiStreamProcessor::addETIStream(const QString& file_path, const QString& stream_name)
{
    QMutexLocker locker(&m_streams_mutex);
    
    // Validate ETI file
    if (!isValidETIFile(file_path)) {
        emit streamProcessingError("", QString("Invalid ETI file: %1").arg(file_path));
        return QString();
    }
    
    // Generate unique stream ID
    QString stream_id = generateStreamId(file_path);
    
    // Create stream info
    ETIStreamInfo stream_info;
    stream_info.stream_id = stream_id;
    stream_info.file_path = file_path;
    stream_info.stream_name = stream_name.isEmpty() ? QFileInfo(file_path).baseName() : stream_name;
    stream_info.load_time = QDateTime::currentDateTime();
    
    // Get file information
    QFileInfo file_info(file_path);
    stream_info.file_size = file_info.size();
    stream_info.total_frames = static_cast<uint32_t>(stream_info.file_size / 6144); // ETI frame size
    stream_info.is_active = false;
    stream_info.processing_progress = 0.0;
    
    // Add to active streams
    m_active_streams[stream_id] = stream_info;
    
    // Initialize statistics
    StreamProcessingStats stats;
    stats.stream_id = stream_id;
    m_stream_stats[stream_id] = stats;
    
    emit streamAdded(stream_id, stream_info);
    
    qDebug() << "Added ETI stream:" << stream_name << "(" << stream_id << ")" 
             << "Size:" << stream_info.file_size << "bytes"
             << "Estimated frames:" << stream_info.total_frames;
    
    return stream_id;
}

bool MultiStreamProcessor::removeETIStream(const QString& stream_id)
{
    QMutexLocker locker(&m_streams_mutex);
    
    // Stop processing if active
    if (m_processing_threads.find(stream_id) != m_processing_threads.end()) {
        m_processing_threads[stream_id]->quit();
        m_processing_threads[stream_id]->wait(3000);
        m_processing_threads.erase(stream_id);
    }
    
    // Remove from collections
    auto stream_it = m_active_streams.find(stream_id);
    if (stream_it != m_active_streams.end()) {
        m_active_streams.erase(stream_it);
        m_stream_stats.erase(stream_id);
        
        emit streamRemoved(stream_id);
        return true;
    }
    
    return false;
}

void MultiStreamProcessor::clearAllStreams()
{
    stopParallelProcessing();
    
    QMutexLocker locker(&m_streams_mutex);
    
    std::vector<QString> stream_ids;
    for (const auto& pair : m_active_streams) {
        stream_ids.push_back(pair.first);
    }
    
    locker.unlock();
    
    for (const QString& stream_id : stream_ids) {
        removeETIStream(stream_id);
    }
}

void MultiStreamProcessor::startParallelProcessing()
{
    if (m_processing_active.load()) {
        qDebug() << "Multi-stream processing already active";
        return;
    }
    
    QMutexLocker locker(&m_streams_mutex);
    
    if (m_active_streams.empty()) {
        qDebug() << "No streams available for processing";
        return;
    }
    
    m_processing_active.store(true);
    
    // Start processing for each stream (up to max parallel limit)
    int started_streams = 0;
    for (auto& pair : m_active_streams) {
        if (started_streams >= m_max_parallel_streams.load()) {
            break;
        }
        
        const QString& stream_id = pair.first;
        ETIStreamInfo& stream_info = pair.second;
        
        // Create processing thread
        auto thread = std::make_unique<QThread>();
        auto worker = new StreamProcessingWorker(stream_id, stream_info);
        worker->moveToThread(thread.get());
        
        // Connect signals
        connect(thread.get(), &QThread::started, worker, &StreamProcessingWorker::processStream);
        connect(worker, &StreamProcessingWorker::progressUpdated, 
                this, &MultiStreamProcessor::streamProcessingProgress);
        connect(worker, &StreamProcessingWorker::processingCompleted,
                this, &MultiStreamProcessor::streamProcessingCompleted);
        connect(worker, &StreamProcessingWorker::processingError,
                this, &MultiStreamProcessor::streamProcessingError);
        connect(worker, &StreamProcessingWorker::processingCompleted,
                thread.get(), &QThread::quit);
        connect(thread.get(), &QThread::finished, worker, &QObject::deleteLater);
        
        // Start thread
        m_processing_threads[stream_id] = std::move(thread);
        m_processing_threads[stream_id]->start();
        
        stream_info.is_active = true;
        emit streamProcessingStarted(stream_id);
        
        started_streams++;
    }
    
    // Start monitoring timer
    m_processing_timer->start();
    
    qDebug() << "Started parallel processing for" << started_streams << "streams";
}

void MultiStreamProcessor::stopParallelProcessing()
{
    if (!m_processing_active.load()) {
        return;
    }
    
    m_processing_active.store(false);
    m_processing_timer->stop();
    
    QMutexLocker locker(&m_streams_mutex);
    
    // Stop all processing threads
    for (auto& pair : m_processing_threads) {
        pair.second->quit();
        if (!pair.second->wait(5000)) {
            pair.second->terminate();
            pair.second->wait(1000);
        }
    }
    
    m_processing_threads.clear();
    
    // Mark all streams as inactive
    for (auto& pair : m_active_streams) {
        pair.second.is_active = false;
    }
    
    qDebug() << "Stopped parallel processing for all streams";
}

std::vector<ETIStreamInfo> MultiStreamProcessor::getActiveStreams() const
{
    QMutexLocker locker(&m_streams_mutex);
    
    std::vector<ETIStreamInfo> streams;
    streams.reserve(m_active_streams.size());
    
    for (const auto& pair : m_active_streams) {
        streams.push_back(pair.second);
    }
    
    return streams;
}

ETIStreamInfo MultiStreamProcessor::getStreamInfo(const QString& stream_id) const
{
    QMutexLocker locker(&m_streams_mutex);
    
    auto it = m_active_streams.find(stream_id);
    if (it != m_active_streams.end()) {
        return it->second;
    }
    
    return ETIStreamInfo(); // Return empty info if not found
}

StreamProcessingStats MultiStreamProcessor::getStreamStats(const QString& stream_id) const
{
    QMutexLocker locker(&m_streams_mutex);
    
    auto it = m_stream_stats.find(stream_id);
    if (it != m_stream_stats.end()) {
        return it->second;
    }
    
    return StreamProcessingStats(); // Return empty stats if not found
}

ComparativeAnalysisResult MultiStreamProcessor::compareStreams(const std::vector<QString>& stream_ids)
{
    ComparativeAnalysisResult result;
    result.analysis_id = QUuid::createUuid().toString(QUuid::WithoutBraces);
    result.analysis_time = QDateTime::currentDateTime();
    result.compared_streams = stream_ids;
    
    // Analyze frame sequences
    analyzeFrameSequences(stream_ids, result);
    
    // Analyze service discovery
    analyzeServiceDiscovery(stream_ids, result);
    
    // Calculate quality scores
    for (const QString& stream_id : stream_ids) {
        result.quality_scores[stream_id] = calculateStreamQuality(stream_id);
    }
    
    // Check synchronization
    result.streams_synchronized = areStreamsSynchronized(stream_ids);
    result.synchronization_report = generateSynchronizationReport(stream_ids);
    
    emit comparativeAnalysisCompleted(result);
    
    return result;
}

ComparativeAnalysisResult MultiStreamProcessor::compareAllStreams()
{
    QMutexLocker locker(&m_streams_mutex);
    
    std::vector<QString> all_stream_ids;
    for (const auto& pair : m_active_streams) {
        all_stream_ids.push_back(pair.first);
    }
    
    locker.unlock();
    
    return compareStreams(all_stream_ids);
}

bool MultiStreamProcessor::areStreamsSynchronized(const std::vector<QString>& stream_ids)
{
    if (stream_ids.size() < 2) {
        return true; // Single stream is always synchronized
    }
    
    QMutexLocker locker(&m_streams_mutex);
    
    // Check if all streams have similar frame counts (within 5% tolerance)
    std::vector<uint32_t> frame_counts;
    for (const QString& stream_id : stream_ids) {
        auto it = m_active_streams.find(stream_id);
        if (it != m_active_streams.end()) {
            frame_counts.push_back(it->second.total_frames);
        }
    }
    
    if (frame_counts.empty()) {
        return false;
    }
    
    uint32_t min_frames = *std::min_element(frame_counts.begin(), frame_counts.end());
    uint32_t max_frames = *std::max_element(frame_counts.begin(), frame_counts.end());
    
    double tolerance = 0.05; // 5% tolerance
    double difference = static_cast<double>(max_frames - min_frames) / min_frames;
    
    return difference <= tolerance;
}

QString MultiStreamProcessor::generateSynchronizationReport(const std::vector<QString>& stream_ids)
{
    QString report;
    
    if (stream_ids.size() < 2) {
        return "Single stream - synchronization not applicable";
    }
    
    QMutexLocker locker(&m_streams_mutex);
    
    report += QString("Synchronization Report (%1 streams):\n").arg(stream_ids.size());
    report += QString("Analysis Time: %1\n\n").arg(QDateTime::currentDateTime().toString());
    
    // Frame count analysis
    report += "Frame Count Analysis:\n";
    uint32_t min_frames = UINT32_MAX;
    uint32_t max_frames = 0;
    
    for (const QString& stream_id : stream_ids) {
        auto it = m_active_streams.find(stream_id);
        if (it != m_active_streams.end()) {
            const ETIStreamInfo& info = it->second;
            report += QString("  %1: %2 frames\n").arg(info.stream_name).arg(info.total_frames);
            min_frames = std::min(min_frames, info.total_frames);
            max_frames = std::max(max_frames, info.total_frames);
        }
    }
    
    if (min_frames != UINT32_MAX) {
        double difference_percent = (static_cast<double>(max_frames - min_frames) / min_frames) * 100.0;
        report += QString("\nFrame count difference: %1% (%2 frames)\n")
                 .arg(difference_percent, 0, 'f', 2).arg(max_frames - min_frames);
        
        if (difference_percent <= 5.0) {
            report += "Status: SYNCHRONIZED (within 5% tolerance)\n";
        } else {
            report += "Status: NOT SYNCHRONIZED (exceeds 5% tolerance)\n";
        }
    }
    
    return report;
}

void MultiStreamProcessor::onProcessingTimerTimeout()
{
    if (!m_processing_active.load()) {
        return;
    }
    
    // Update processing progress and perform comparative analysis if enabled
    if (m_comparative_analysis_enabled.load()) {
        performComparativeAnalysis();
    }
}

QString MultiStreamProcessor::generateStreamId(const QString& file_path)
{
    QString base_name = QFileInfo(file_path).baseName();
    QString timestamp = QString::number(QDateTime::currentMSecsSinceEpoch());
    return QString("%1_%2").arg(base_name, timestamp.right(6));
}

bool MultiStreamProcessor::isValidETIFile(const QString& file_path)
{
    QFile file(file_path);
    if (!file.open(QIODevice::ReadOnly)) {
        return false;
    }
    
    // Check minimum file size (at least one ETI frame)
    if (file.size() < 6144) {
        return false;
    }
    
    // Check ETI sync bytes at the beginning
    QByteArray header = file.read(4);
    if (header.size() != 4) {
        return false;
    }
    
    // ETI sync pattern: 0x68, 0x1A, 0x4B, 0x1C
    return (static_cast<uint8_t>(header[0]) == 0x68 &&
            static_cast<uint8_t>(header[1]) == 0x1A &&
            static_cast<uint8_t>(header[2]) == 0x4B &&
            static_cast<uint8_t>(header[3]) == 0x1C);
}

bool MultiStreamProcessor::analyzeFrameSequences(const std::vector<QString>& stream_ids, ComparativeAnalysisResult& result)
{
    QMutexLocker locker(&m_streams_mutex);
    
    for (const QString& stream_id : stream_ids) {
        auto it = m_active_streams.find(stream_id);
        if (it != m_active_streams.end()) {
            const ETIStreamInfo& info = it->second;
            result.frame_counts[stream_id] = info.total_frames;
            
            // Generate sample frame counter sequence (simulation)
            std::vector<uint32_t> sequence;
            for (uint32_t i = 0; i < std::min(100u, info.total_frames); i++) {
                sequence.push_back(i);
            }
            result.frame_counter_sequences[stream_id] = sequence;
        }
    }
    
    return true;
}

bool MultiStreamProcessor::analyzeServiceDiscovery(const std::vector<QString>& stream_ids, ComparativeAnalysisResult& result)
{
    // Simulate service discovery comparison
    for (const QString& stream_id : stream_ids) {
        std::vector<QString> services;
        services.push_back("NBT Radio");
        services.push_back("NBT News");
        services.push_back("Traffic Info");
        
        result.discovered_services[stream_id] = services;
    }
    
    return true;
}

double MultiStreamProcessor::calculateStreamQuality(const QString& stream_id)
{
    QMutexLocker locker(&m_streams_mutex);
    
    auto stats_it = m_stream_stats.find(stream_id);
    if (stats_it == m_stream_stats.end()) {
        return 0.0;
    }
    
    const StreamProcessingStats& stats = stats_it->second;
    
    // Calculate quality score based on processing success rate
    if (stats.frames_processed == 0) {
        return 0.0;
    }
    
    double success_rate = 1.0 - (static_cast<double>(stats.processing_errors) / stats.frames_processed);
    return std::max(0.0, std::min(100.0, success_rate * 100.0));
}

void MultiStreamProcessor::performComparativeAnalysis()
{
    if (!m_comparative_analysis_enabled.load()) {
        return;
    }
    
    // Perform periodic comparative analysis
    ComparativeAnalysisResult result = compareAllStreams();
    
    // Check synchronization status
    bool synchronized = result.streams_synchronized;
    emit synchronizationStatusChanged(synchronized);
}

// StreamProcessingWorker Implementation

StreamProcessingWorker::StreamProcessingWorker(const QString& stream_id, const ETIStreamInfo& stream_info, QObject* parent)
    : QObject(parent), m_stream_id(stream_id), m_stream_info(stream_info)
{
}

void StreamProcessingWorker::processStream()
{
    try {
        StreamProcessingStats stats = processETIFrames();
        emit processingCompleted(m_stream_id, stats);
    } catch (const std::exception& e) {
        emit processingError(m_stream_id, QString("Processing error: %1").arg(e.what()));
    } catch (...) {
        emit processingError(m_stream_id, "Unknown processing error");
    }
}

StreamProcessingStats StreamProcessingWorker::processETIFrames()
{
    StreamProcessingStats stats;
    stats.stream_id = m_stream_id;
    stats.last_frame_time = QDateTime::currentDateTime();
    
    auto start_time = std::chrono::steady_clock::now();
    
    QFile file(m_stream_info.file_path);
    if (!file.open(QIODevice::ReadOnly)) {
        throw std::runtime_error("Cannot open ETI file");
    }
    
    const int FRAME_SIZE = 6144;
    std::vector<uint8_t> frame_buffer(FRAME_SIZE);
    uint32_t frame_index = 0;
    
    while (!file.atEnd()) {
        qint64 bytes_read = file.read(reinterpret_cast<char*>(frame_buffer.data()), FRAME_SIZE);
        if (bytes_read != FRAME_SIZE) {
            break; // End of file or incomplete frame
        }
        
        // Validate frame
        if (validateETIFrame(frame_buffer, frame_index)) {
            stats.frames_processed++;
        } else {
            stats.processing_errors++;
        }
        
        stats.bytes_processed += bytes_read;
        frame_index++;
        
        // Update progress periodically
        if (frame_index % 100 == 0) {
            double progress = static_cast<double>(frame_index) / m_stream_info.total_frames * 100.0;
            emit progressUpdated(m_stream_id, progress);
        }
    }
    
    auto end_time = std::chrono::steady_clock::now();
    stats.processing_time = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
    
    // Calculate FPS
    if (stats.processing_time.count() > 0) {
        stats.fps_rate = (static_cast<double>(stats.frames_processed) / stats.processing_time.count()) * 1000.0;
    }
    
    return stats;
}

bool StreamProcessingWorker::validateETIFrame(const std::vector<uint8_t>& frame_data, uint32_t frame_index)
{
    if (frame_data.size() < 4) {
        return false;
    }
    
    // Validate ETI sync bytes
    return (frame_data[0] == 0x68 && frame_data[1] == 0x1A && 
            frame_data[2] == 0x4B && frame_data[3] == 0x1C);
}

void MultiStreamProcessor::onStreamProcessingFinished()
{
    // Handle completion of stream processing
    // This slot is called when a processing thread finishes
    QMutexLocker locker(&m_streams_mutex);
    
    // Check if all streams have finished processing by checking if they are still active
    bool all_finished = true;
    for (const auto& pair : m_active_streams) {
        if (pair.second.is_active && pair.second.processing_progress < 100.0) {
            all_finished = false;
            break;
        }
    }
    
    if (all_finished && m_processing_active.load()) {
        m_processing_active.store(false);
        qDebug() << "All streams processing completed";
        
        // Emit comparative analysis if enabled
        if (m_comparative_analysis_enabled.load()) {
            ComparativeAnalysisResult result = compareAllStreams();
            emit comparativeAnalysisCompleted(result);
        }
    }
}

} // namespace multistream