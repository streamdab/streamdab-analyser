#include "multi_stream_processor.hpp"
#include <QFile>
#include <QFileInfo>
#include <QUuid>
#include <QDebug>
#include <QStandardPaths>
#include <algorithm>
#include <numeric>

namespace multistream {

// MultiStreamWorker implementation
MultiStreamWorker::MultiStreamWorker(QObject* parent)
    : QObject(parent)
{
    // Initialize timers - these will run in the worker thread
    m_processing_timer = new QTimer(this);
    m_analysis_timer = new QTimer(this);

    // Connect signals
    connect(m_processing_timer, &QTimer::timeout, this, &MultiStreamWorker::processStreams);
    connect(m_analysis_timer, &QTimer::timeout, this, &MultiStreamWorker::performComparativeAnalysis);
}

void MultiStreamWorker::startProcessing()
{
    if (!m_processing_active) {
        m_processing_active = true;

        // Start timers with reasonable intervals (background thread can handle this)
        m_processing_timer->start(1000);   // 1 second for stream processing
        m_analysis_timer->start(5000);     // 5 seconds for comparative analysis

        qDebug() << "MultiStreamWorker: Background processing started in thread" << QThread::currentThread();
    }
}

void MultiStreamWorker::stopProcessing()
{
    m_processing_active = false;
    m_processing_timer->stop();
    m_analysis_timer->stop();
    qDebug() << "MultiStreamWorker: Background processing stopped";
}

void MultiStreamWorker::processStreams()
{
    if (!m_processing_active) return;

    // Background processing - doesn't block UI
    // TODO: Add actual stream processing logic here
    // For now, just emit progress signals
}

void MultiStreamWorker::performComparativeAnalysis()
{
    if (!m_processing_active) return;

    // Background processing - doesn't block UI
    // TODO: Add actual comparative analysis here
    ComparativeAnalysisResult result;
    emit comparativeAnalysisCompleted(result);
}

MultiStreamProcessor::MultiStreamProcessor(QObject* parent)
    : QObject(parent)
    , m_processing_active(false)
    , m_max_parallel_streams(4)
    , m_processing_thread_count(4)
    , m_comparative_analysis_enabled(true)
{
    // Initialize worker pointers (create workers only when needed)
    m_worker_thread = nullptr;
    m_worker = nullptr;

    qDebug() << "MultiStreamProcessor initialized with QThread background processing";
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
    auto worker_it = m_processing_workers.find(stream_id);
    if (worker_it != m_processing_workers.end()) {
        if (worker_it->second.thread && worker_it->second.thread->isRunning()) {
            worker_it->second.thread->quit();

            if (!worker_it->second.thread->wait(5000)) {
                qWarning() << "Thread for stream" << stream_id << "did not stop, terminating";
                worker_it->second.thread->terminate();
                worker_it->second.thread->wait(1000);
            }
        }
        m_processing_workers.erase(worker_it);
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

        // Create worker context with proper ownership
        WorkerContext context;
        context.stream_id = stream_id;
        context.thread = std::make_unique<QThread>();

        // Create worker - will be owned by thread through Qt parent-child relationship
        context.worker = new StreamProcessingWorker(stream_id, stream_info);
        context.worker->moveToThread(context.thread.get());

        // Connect signals BEFORE starting thread
        connect(context.thread.get(), &QThread::started,
                context.worker, &StreamProcessingWorker::processStream,
                Qt::QueuedConnection);

        connect(context.worker, &StreamProcessingWorker::progressUpdated,
                this, &MultiStreamProcessor::streamProcessingProgress,
                Qt::QueuedConnection);

        connect(context.worker, &StreamProcessingWorker::processingCompleted,
                this, &MultiStreamProcessor::streamProcessingCompleted,
                Qt::QueuedConnection);

        connect(context.worker, &StreamProcessingWorker::processingError,
                this, &MultiStreamProcessor::streamProcessingError,
                Qt::QueuedConnection);

        connect(context.worker, &StreamProcessingWorker::processingCompleted,
                context.thread.get(), &QThread::quit,
                Qt::QueuedConnection);

        connect(context.thread.get(), &QThread::finished,
                context.worker, &QObject::deleteLater,
                Qt::DirectConnection);

        // Store context
        m_processing_workers[stream_id] = std::move(context);

        // Start thread
        m_processing_workers[stream_id].thread->start();

        stream_info.is_active = true;
        emit streamProcessingStarted(stream_id);

        started_streams++;
    }

    // Lazy initialization - create worker only when first needed
    initializeWorkerIfNeeded();

    // Start worker thread processing
    if (m_worker_thread && !m_worker_thread->isRunning()) {
        m_worker_thread->start();
    }

    qDebug() << "Started parallel processing for" << started_streams << "streams";
}

void MultiStreamProcessor::initializeWorkerIfNeeded()
{
    if (!m_worker_thread) {
        // Create background thread and worker (only when first needed)
        m_worker_thread = new QThread(this);
        m_worker = new MultiStreamWorker();
        m_worker->moveToThread(m_worker_thread);

        // Connect worker signals to main thread (thread-safe)
        connect(m_worker, &MultiStreamWorker::streamProcessingProgress,
                this, &MultiStreamProcessor::streamProcessingProgress, Qt::QueuedConnection);
        connect(m_worker, &MultiStreamWorker::streamProcessingCompleted,
                this, &MultiStreamProcessor::streamProcessingCompleted, Qt::QueuedConnection);
        connect(m_worker, &MultiStreamWorker::comparativeAnalysisCompleted,
                this, &MultiStreamProcessor::comparativeAnalysisCompleted, Qt::QueuedConnection);
        connect(m_worker, &MultiStreamWorker::processingError,
                this, &MultiStreamProcessor::streamProcessingError, Qt::QueuedConnection);

        // Connect thread lifecycle
        connect(m_worker_thread, &QThread::finished, m_worker, &QObject::deleteLater);

        qDebug() << "MultiStreamProcessor: Worker thread created on-demand";
    }
}

void MultiStreamProcessor::stopParallelProcessing()
{
    if (!m_processing_active.load()) {
        return;
    }

    m_processing_active.store(false);

    // Stop worker processing
    if (m_worker) {
        QMetaObject::invokeMethod(m_worker, "stopProcessing", Qt::QueuedConnection);
    }

    QMutexLocker locker(&m_streams_mutex);

    // Stop all processing workers
    for (auto& pair : m_processing_workers) {
        if (pair.second.thread && pair.second.thread->isRunning()) {
            pair.second.thread->quit();

            if (!pair.second.thread->wait(5000)) {
                qWarning() << "Worker thread" << pair.first << "did not stop, terminating";
                pair.second.thread->terminate();
                pair.second.thread->wait(1000);
            }
        }
    }

    m_processing_workers.clear();

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

    // ETI sync pattern validation (ETSI EN 300 799)
    // Extract SYNC pattern (big-endian)
    uint32_t sync = (static_cast<uint32_t>(static_cast<uint8_t>(header[0])) << 24) |
                    (static_cast<uint32_t>(static_cast<uint8_t>(header[1])) << 16) |
                    (static_cast<uint32_t>(static_cast<uint8_t>(header[2])) << 8) |
                    static_cast<uint32_t>(static_cast<uint8_t>(header[3]));

    // Valid SYNC patterns: ETI-NI and ETI-LI
    bool is_eti_ni = (sync == 0xFF1F491F || sync == 0x491F1FFF ||
                      sync == 0xFF1FC4FF || sync == 0xC4FF1FFF ||
                      sync == 0x491FC4FF || sync == 0xC4FF491F ||
                      sync == 0x49931E03);
    bool is_eti_li = (sync == 0xFFF8C549 || sync == 0x49C5F8FF ||
                      sync == 0xFF073AB6 || sync == 0xB63A07FF);

    return is_eti_ni || is_eti_li;
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
