#pragma once

#include <QObject>
#include <QString>
#include <QDateTime>
#include <QThread>
#include <QMutex>
#include <QTimer>
#include <vector>
#include <memory>
#include <map>
#include <atomic>
#include <future>

namespace multistream {

// ETI Stream Information
struct ETIStreamInfo {
    QString stream_id;
    QString file_path;
    QString stream_name;
    QDateTime load_time;
    uint32_t total_frames;
    uint64_t file_size;
    bool is_active;
    double processing_progress;

    ETIStreamInfo() : total_frames(0), file_size(0), is_active(false), processing_progress(0.0) {}
};

// Stream Processing Statistics
struct StreamProcessingStats {
    QString stream_id;
    uint32_t frames_processed;
    uint32_t processing_errors;
    double fps_rate;
    uint64_t bytes_processed;
    QDateTime last_frame_time;
    std::chrono::milliseconds processing_time;

    StreamProcessingStats() : frames_processed(0), processing_errors(0),
                             fps_rate(0.0), bytes_processed(0) {}
};

// Comparative Analysis Result
struct ComparativeAnalysisResult {
    QString analysis_id;
    QDateTime analysis_time;
    std::vector<QString> compared_streams;

    // Frame count comparison
    std::map<QString, uint32_t> frame_counts;

    // Service discovery comparison
    std::map<QString, std::vector<QString>> discovered_services;

    // Quality metrics comparison
    std::map<QString, double> quality_scores;

    // Synchronization analysis
    std::map<QString, std::vector<uint32_t>> frame_counter_sequences;
    bool streams_synchronized;
    QString synchronization_report;

    ComparativeAnalysisResult() : streams_synchronized(false) {}
};

// QThread Worker for background multi-stream processing
class MultiStreamWorker : public QObject {
    Q_OBJECT

public:
    explicit MultiStreamWorker(QObject* parent = nullptr);
    ~MultiStreamWorker() = default;

public slots:
    void startProcessing();
    void stopProcessing();
    void processStreams();
    void performComparativeAnalysis();

signals:
    void streamProcessingProgress(const QString& streamId, double progress);
    void streamProcessingCompleted(const QString& streamId, const StreamProcessingStats& stats);
    void comparativeAnalysisCompleted(const ComparativeAnalysisResult& result);
    void processingError(const QString& streamId, const QString& error);

private:
    std::atomic<bool> m_processing_active{false};
    QTimer* m_processing_timer{nullptr};
    QTimer* m_analysis_timer{nullptr};
};

// Stream Processing Worker (for threading) - Forward declaration
class StreamProcessingWorker;

// Multi-stream Processing Engine
class MultiStreamProcessor : public QObject {
    Q_OBJECT

public:
    explicit MultiStreamProcessor(QObject* parent = nullptr);
    virtual ~MultiStreamProcessor();

    // Stream management
    QString addETIStream(const QString& file_path, const QString& stream_name = "");
    bool removeETIStream(const QString& stream_id);
    void clearAllStreams();

    // Stream processing
    void startParallelProcessing();
    void stopParallelProcessing();
    void pauseProcessing(const QString& stream_id);
    void resumeProcessing(const QString& stream_id);

    // Stream information
    std::vector<ETIStreamInfo> getActiveStreams() const;
    ETIStreamInfo getStreamInfo(const QString& stream_id) const;
    StreamProcessingStats getStreamStats(const QString& stream_id) const;

    // Comparative analysis
    ComparativeAnalysisResult compareStreams(const std::vector<QString>& stream_ids);
    ComparativeAnalysisResult compareAllStreams();

    // Stream synchronization
    bool areStreamsSynchronized(const std::vector<QString>& stream_ids);
    QString generateSynchronizationReport(const std::vector<QString>& stream_ids);

    // Configuration
    void setMaxParallelStreams(int max_streams);
    void setProcessingThreads(int thread_count);
    void setComparativeAnalysisEnabled(bool enabled);

signals:
    void streamAdded(const QString& stream_id, const ETIStreamInfo& info);
    void streamRemoved(const QString& stream_id);
    void streamProcessingStarted(const QString& stream_id);
    void streamProcessingCompleted(const QString& stream_id, const StreamProcessingStats& stats);
    void streamProcessingProgress(const QString& stream_id, double progress);
    void streamProcessingError(const QString& stream_id, const QString& error);
    void comparativeAnalysisCompleted(const ComparativeAnalysisResult& result);
    void synchronizationStatusChanged(bool all_synchronized);

private slots:
    void onProcessingTimerTimeout();
    void onStreamProcessingFinished();

private:
    // Worker context for thread safety
    struct WorkerContext {
        std::unique_ptr<QThread> thread;
        StreamProcessingWorker* worker; // Worker owned by thread via Qt parent-child
        QString stream_id;

        WorkerContext() : worker(nullptr) {}
        WorkerContext(WorkerContext&&) = default;
        WorkerContext& operator=(WorkerContext&&) = default;
    };

    // Internal stream management
    mutable QMutex m_streams_mutex;
    std::map<QString, ETIStreamInfo> m_active_streams;
    std::map<QString, StreamProcessingStats> m_stream_stats;
    std::map<QString, WorkerContext> m_processing_workers;

    // Processing control
    std::atomic<bool> m_processing_active;
    std::atomic<int> m_max_parallel_streams;
    std::atomic<int> m_processing_thread_count;
    std::atomic<bool> m_comparative_analysis_enabled;

    // QThread-based background processing (replaces blocking timer)
    QThread* m_worker_thread;
    MultiStreamWorker* m_worker;

    // Internal methods
    QString generateStreamId(const QString& file_path);
    bool isValidETIFile(const QString& file_path);
    void initializeWorkerIfNeeded();
    StreamProcessingStats processETIStreamInternal(const QString& stream_id, const ETIStreamInfo& stream_info);
    void updateStreamProgress(const QString& stream_id, double progress);
    void performComparativeAnalysis();

    // Analysis helpers
    bool analyzeFrameSequences(const std::vector<QString>& stream_ids, ComparativeAnalysisResult& result);
    bool analyzeServiceDiscovery(const std::vector<QString>& stream_ids, ComparativeAnalysisResult& result);
    double calculateStreamQuality(const QString& stream_id);
};

// Stream Processing Worker (for threading)
class StreamProcessingWorker : public QObject {
    Q_OBJECT

public:
    explicit StreamProcessingWorker(const QString& stream_id, const ETIStreamInfo& stream_info, QObject* parent = nullptr);

public slots:
    void processStream();

signals:
    void progressUpdated(const QString& stream_id, double progress);
    void processingCompleted(const QString& stream_id, const StreamProcessingStats& stats);
    void processingError(const QString& stream_id, const QString& error);

private:
    QString m_stream_id;
    ETIStreamInfo m_stream_info;

    StreamProcessingStats processETIFrames();
    bool validateETIFrame(const std::vector<uint8_t>& frame_data, uint32_t frame_index);
};

} // namespace multistream
