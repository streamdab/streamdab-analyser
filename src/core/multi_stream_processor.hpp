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
    // Internal stream management
    mutable QMutex m_streams_mutex;
    std::map<QString, ETIStreamInfo> m_active_streams;
    std::map<QString, StreamProcessingStats> m_stream_stats;
    std::map<QString, std::unique_ptr<QThread>> m_processing_threads;
    
    // Processing control
    std::atomic<bool> m_processing_active;
    std::atomic<int> m_max_parallel_streams;
    std::atomic<int> m_processing_thread_count;
    std::atomic<bool> m_comparative_analysis_enabled;
    
    // Monitoring
    std::unique_ptr<QTimer> m_processing_timer;
    std::map<QString, std::future<StreamProcessingStats>> m_processing_futures;
    
    // Internal methods
    QString generateStreamId(const QString& file_path);
    bool isValidETIFile(const QString& file_path);
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