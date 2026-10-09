/**
 * @file stream_playback.h
 * @brief Professional Stream Playback Engine with Advanced Controls
 * 
 * Implements comprehensive stream playback capabilities for:
 * - Professional ETI recording playback with frame-accurate control
 * - Variable speed playback with audio synchronization
 * - Frame-by-frame navigation and analysis mode
 * - Multiple format support (ETI-NI, ETI-LI, EDI, compressed)
 * - Time-based seeking and timestamp synchronization
 * - Professional broadcast workflow integration
 * 
 * @author Network/Stream Agent
 * @date 2025-09-21
 * @copyright Copyright (c) 2025 ETI Stream Analyser Team
 */

#ifndef STREAM_PLAYBACK_H
#define STREAM_PLAYBACK_H

#include <QObject>
#include <QTimer>
#include <QFile>
#include <QDataStream>
#include <QMutex>
#include <QDateTime>
#include <memory>
#include <atomic>
#include <chrono>
#include <functional>
#include <vector>
#include <deque>

#include "../core/eti_types.hpp"
#include "../core/stream_recording_manager.hpp"

namespace eti_network {

/**
 * @brief Playback state enumeration for transport controls
 */
enum class PlaybackState {
    STOPPED,                // Playback stopped
    PLAYING,                // Normal playback
    PAUSED,                 // Playback paused
    SEEKING,                // Seeking to position
    BUFFERING,              // Buffering data
    FAST_FORWARD,           // Fast forward playback
    FAST_REVERSE,           // Fast reverse playback
    FRAME_STEP,             // Frame-by-frame stepping
    ERROR_STATE             // Error occurred
};

/**
 * @brief Playback mode configuration for different use cases
 */
enum class PlaybackMode {
    REAL_TIME,              // Real-time playback with timing constraints
    ANALYSIS,               // Analysis mode with frame accuracy
    FAST_SCAN,              // Fast scanning without full processing
    FRAME_ACCURATE,         // Frame-accurate navigation mode
    BROADCAST_SIMULATION    // Simulate broadcast timing
};

/**
 * @brief Playback configuration for professional controls
 */
struct PlaybackConfig {
    PlaybackMode mode = PlaybackMode::REAL_TIME;
    double speed_multiplier = 1.0;                    // Playback speed (0.1x - 10.0x)
    bool loop_enabled = false;                        // Loop playback
    bool frame_accurate = true;                       // Frame-accurate positioning
    bool maintain_timing = true;                      // Maintain original timing
    
    // Buffer configuration
    size_t buffer_size_frames = 1000;                 // Playback buffer size
    size_t prebuffer_frames = 100;                    // Pre-buffer before playback
    std::chrono::milliseconds buffer_timeout{5000};   // Buffer timeout
    
    // Quality settings
    bool enable_error_correction = true;              // Error correction during playback
    bool skip_corrupted_frames = false;               // Skip corrupted frames
    double quality_threshold = 0.8;                   // Minimum quality threshold
    
    // Synchronization
    bool sync_to_system_clock = false;                // Sync to system clock
    bool sync_to_audio_samples = true;                // Sync to audio sample rate
    std::chrono::microseconds sync_tolerance{1000};   // Synchronization tolerance
    
    // Advanced features
    bool enable_frame_interpolation = false;          // Frame interpolation for smooth playback
    bool enable_quality_analysis = true;              // Real-time quality analysis
    bool enable_performance_monitoring = true;        // Performance monitoring
};

/**
 * @brief Playback statistics and performance metrics
 */
struct PlaybackStatistics {
    // Position information
    std::atomic<qint64> current_frame{0};             // Current frame number
    std::atomic<qint64> total_frames{0};              // Total frames in recording
    std::atomic<double> current_position_sec{0.0};    // Current position in seconds
    std::atomic<double> total_duration_sec{0.0};      // Total duration in seconds
    std::atomic<double> playback_progress{0.0};       // Playback progress (0.0-1.0)
    
    // Playback performance
    std::atomic<double> actual_speed{1.0};            // Actual playback speed
    std::atomic<double> frame_rate{0.0};              // Current frame rate
    std::atomic<qint64> frames_played{0};             // Total frames played
    std::atomic<qint64> frames_skipped{0};            // Frames skipped due to errors
    std::atomic<qint64> frames_repeated{0};           // Frames repeated for timing
    
    // Buffer status
    std::atomic<double> buffer_fill_level{0.0};       // Buffer fill level (0.0-1.0)
    std::atomic<qint64> buffer_underruns{0};          // Buffer underrun count
    std::atomic<qint64> buffer_overruns{0};           // Buffer overrun count
    
    // Quality metrics
    std::atomic<double> average_quality{1.0};         // Average frame quality
    std::atomic<qint64> decode_errors{0};             // Decode error count
    std::atomic<qint64> sync_errors{0};               // Synchronization error count
    std::atomic<qint64> timing_violations{0};         // Timing violation count
    
    // Performance metrics
    std::atomic<double> cpu_usage{0.0};               // CPU usage percentage
    std::atomic<double> memory_usage_mb{0.0};         // Memory usage in MB
    std::atomic<qint64> disk_read_bytes{0};           // Disk read bytes
    std::atomic<double> disk_read_rate_mbps{0.0};     // Disk read rate in MB/s
    
    void reset() {
        current_frame = 0;
        total_frames = 0;
        current_position_sec = 0.0;
        total_duration_sec = 0.0;
        playback_progress = 0.0;
        actual_speed = 1.0;
        frame_rate = 0.0;
        frames_played = 0;
        frames_skipped = 0;
        frames_repeated = 0;
        buffer_fill_level = 0.0;
        buffer_underruns = 0;
        buffer_overruns = 0;
        average_quality = 1.0;
        decode_errors = 0;
        sync_errors = 0;
        timing_violations = 0;
        cpu_usage = 0.0;
        memory_usage_mb = 0.0;
        disk_read_bytes = 0;
        disk_read_rate_mbps = 0.0;
    }
};

/**
 * @brief Playback frame with timing and quality information
 */
struct PlaybackFrame {
    eti::EtiFrame frame;                              // ETI frame data
    qint64 frame_number = 0;                          // Frame sequence number
    QDateTime timestamp;                              // Original timestamp
    std::chrono::microseconds playback_time{0};      // Playback timing
    
    // Quality information
    double quality_score = 1.0;                      // Frame quality (0.0-1.0)
    bool has_errors = false;                          // Frame has errors
    QString error_description;                        // Error description
    
    // Playback metadata
    double speed_multiplier = 1.0;                   // Playback speed
    bool interpolated = false;                        // Frame was interpolated
    bool repeated = false;                            // Frame was repeated for timing
    
    // Analysis results (if enabled)
    struct AnalysisData {
        bool fic_valid = true;                        // FIC data is valid
        size_t service_count = 0;                     // Number of services
        double signal_quality = 1.0;                 // Signal quality
        QStringList detected_errors;                 // Detected errors
    } analysis;
    
    bool is_valid() const {
        return frame.is_valid() && !has_errors && quality_score >= 0.5;
    }
    
    std::chrono::microseconds get_expected_duration() const {
        // ETI frames are typically 24ms (41.67 fps)
        return std::chrono::microseconds{24000};
    }
    
    std::chrono::microseconds get_playback_duration() const {
        return std::chrono::microseconds{
            static_cast<int64_t>(get_expected_duration().count() / speed_multiplier)
        };
    }
};

/**
 * @brief Timing engine for accurate playback synchronization
 */
class TimingEngine {
public:
    explicit TimingEngine(const PlaybackConfig& config);
    ~TimingEngine() = default;
    
    // Timing control
    void start_timing();
    void stop_timing();
    void pause_timing();
    void resume_timing();
    void reset_timing();
    
    // Speed control
    void set_speed_multiplier(double speed);
    double get_speed_multiplier() const;
    void set_frame_rate(double fps);
    double get_frame_rate() const;
    
    // Synchronization
    std::chrono::microseconds get_next_frame_delay() const;
    bool should_play_frame() const;
    void frame_played();
    void frame_skipped();
    
    // Statistics
    double get_actual_speed() const;
    qint64 get_timing_violations() const;
    std::chrono::microseconds get_average_jitter() const;
    
private:
    void update_timing_statistics();
    void adjust_timing_accuracy();
    
    PlaybackConfig m_config;
    std::atomic<double> m_speed_multiplier{1.0};
    std::atomic<double> m_frame_rate{41.67}; // Default ETI frame rate
    
    std::chrono::steady_clock::time_point m_start_time;
    std::chrono::steady_clock::time_point m_last_frame_time;
    std::atomic<qint64> m_frames_played{0};
    std::atomic<qint64> m_timing_violations{0};
    
    // Timing accuracy tracking
    std::deque<std::chrono::microseconds> m_jitter_history;
    std::atomic<double> m_actual_speed{1.0};
    
    static constexpr size_t JITTER_HISTORY_SIZE = 100;
    static constexpr std::chrono::microseconds DEFAULT_FRAME_DURATION{24000}; // 24ms
};

/**
 * @brief Frame buffer for smooth playback and seeking
 */
class FrameBuffer {
public:
    explicit FrameBuffer(size_t capacity = 1000);
    ~FrameBuffer() = default;
    
    // Buffer management
    void set_capacity(size_t capacity);
    size_t get_capacity() const;
    size_t get_size() const;
    double get_fill_level() const;
    
    // Frame operations
    void add_frame(const PlaybackFrame& frame);
    PlaybackFrame get_next_frame();
    bool has_frames() const;
    void clear();
    
    // Seeking operations
    bool seek_to_frame(qint64 frame_number);
    bool seek_to_time(std::chrono::microseconds time);
    PlaybackFrame get_frame_at(qint64 frame_number);
    
    // Buffer monitoring
    qint64 get_underrun_count() const;
    qint64 get_overrun_count() const;
    void reset_statistics();
    
private:
    void ensure_capacity();
    void update_statistics();
    
    mutable QMutex m_mutex;
    std::deque<PlaybackFrame> m_frames;
    size_t m_capacity;
    qint64 m_current_position = 0;
    
    // Statistics
    std::atomic<qint64> m_underrun_count{0};
    std::atomic<qint64> m_overrun_count{0};
    std::atomic<qint64> m_total_frames_added{0};
    std::atomic<qint64> m_total_frames_removed{0};
};

/**
 * @brief Playback controller for transport control operations
 */
class PlaybackController {
public:
    explicit PlaybackController(const PlaybackConfig& config);
    ~PlaybackController() = default;
    
    // Transport controls
    void play();
    void pause();
    void stop();
    void step_forward();
    void step_backward();
    void fast_forward(double speed = 2.0);
    void fast_reverse(double speed = 2.0);
    
    // Position control
    bool seek_to_frame(qint64 frame_number);
    bool seek_to_time(std::chrono::microseconds time);
    bool seek_to_position(double position); // 0.0-1.0
    void seek_relative(qint64 frame_offset);
    
    // Speed control
    void set_speed(double speed);
    double get_speed() const;
    void reset_speed();
    
    // Loop control
    void set_loop_enabled(bool enabled);
    bool is_loop_enabled() const;
    void set_loop_region(qint64 start_frame, qint64 end_frame);
    
    // State management
    PlaybackState get_state() const;
    bool is_playing() const;
    bool is_paused() const;
    bool is_stopped() const;
    bool is_seeking() const;
    
    // Position information
    qint64 get_current_frame() const;
    qint64 get_total_frames() const;
    double get_current_position() const;
    std::chrono::microseconds get_current_time() const;
    std::chrono::microseconds get_total_duration() const;
    
private:
    void update_state(PlaybackState new_state);
    void validate_seek_position(qint64& frame_number);
    void handle_loop_boundary();
    
    PlaybackConfig m_config;
    std::atomic<PlaybackState> m_state{PlaybackState::STOPPED};
    std::atomic<double> m_speed{1.0};
    std::atomic<qint64> m_current_frame{0};
    std::atomic<qint64> m_total_frames{0};
    std::atomic<bool> m_loop_enabled{false};
    
    // Loop region
    qint64 m_loop_start_frame = 0;
    qint64 m_loop_end_frame = 0;
    
    mutable QMutex m_state_mutex;
};

/**
 * @brief Professional Stream Playback Engine
 * 
 * Comprehensive stream playback system implementing:
 * - Frame-accurate playback with professional transport controls
 * - Variable speed playback with audio synchronization
 * - Multiple format support and error handling
 * - Professional broadcast workflow integration
 * - Real-time performance monitoring and quality analysis
 */
class StreamPlayback : public QObject {
    Q_OBJECT
    
public:
    explicit StreamPlayback(QObject* parent = nullptr);
    explicit StreamPlayback(const PlaybackConfig& config, QObject* parent = nullptr);
    ~StreamPlayback();
    
    // File management
    bool loadRecording(const QString& filename);
    bool loadRecordingInfo(const QString& filename);
    void closeRecording();
    bool isRecordingLoaded() const;
    QString getCurrentRecording() const;
    
    // Playback control
    void startPlayback();
    void stopPlayback();
    void pausePlayback();
    void resumePlayback();
    bool isPlaying() const;
    bool isPaused() const;
    
    // Transport controls
    void stepForward();
    void stepBackward();
    void fastForward(double speed = 2.0);
    void fastReverse(double speed = 2.0);
    void setPlaybackSpeed(double speedMultiplier);
    double getPlaybackSpeed() const;
    
    // Position control
    bool seekToFrame(qint64 frameNumber);
    bool seekToTime(std::chrono::microseconds time);
    bool seekToPosition(double position); // 0.0-1.0
    void seekRelative(qint64 frameOffset);
    
    // Loop control
    void enableLooping(bool enabled);
    bool isLoopingEnabled() const;
    void setLoopRegion(qint64 startFrame, qint64 endFrame);
    void clearLoopRegion();
    
    // Configuration
    void setConfiguration(const PlaybackConfig& config);
    PlaybackConfig getConfiguration() const;
    void setPlaybackMode(PlaybackMode mode);
    PlaybackMode getPlaybackMode() const;
    void setFrameSkipping(bool enabled);
    void setQualityThreshold(double threshold);
    
    // Information access
    qint64 getCurrentFrame() const;
    qint64 getTotalFrames() const;
    double getCurrentPosition() const;
    std::chrono::microseconds getCurrentTime() const;
    std::chrono::microseconds getTotalDuration() const;
    PlaybackState getPlaybackState() const;
    
    // Statistics and monitoring
    PlaybackStatistics getStatistics() const;
    double getBufferFillLevel() const;
    double getActualPlaybackSpeed() const;
    qint64 getFramesPlayed() const;
    qint64 getPlaybackErrors() const;
    bool isQualityAcceptable() const;
    
    // Format support
    QStringList getSupportedFormats() const;
    eti::RecordingFormat getRecordingFormat() const;
    QString getFormatDescription() const;
    bool validateRecordingIntegrity() const;
    
    // Performance monitoring
    void enablePerformanceMonitoring(bool enabled);
    QString getPerformanceReport() const;
    void resetStatistics();
    void exportPlaybackLog(const QString& filename) const;
    
    // Callback registration
    using FrameCallback = std::function<void(const PlaybackFrame&)>;
    using PositionCallback = std::function<void(qint64 currentFrame, qint64 totalFrames)>;
    using StateCallback = std::function<void(PlaybackState state)>;
    using ErrorCallback = std::function<void(const QString& error)>;
    
    void setFrameCallback(FrameCallback callback);
    void setPositionCallback(PositionCallback callback);
    void setStateCallback(StateCallback callback);
    void setErrorCallback(ErrorCallback callback);

signals:
    void frameReady(const PlaybackFrame& frame);
    void playbackPositionChanged(qint64 currentFrame, qint64 totalFrames);
    void playbackStateChanged(PlaybackState state);
    void playbackStarted();
    void playbackStopped();
    void playbackPaused();
    void playbackResumed();
    void playbackFinished();
    void seekCompleted(qint64 frameNumber);
    void loopCompleted();
    void playbackError(const QString& error);
    void bufferUnderrun();
    void bufferOverrun();
    void qualityDegradation(double quality);
    void performanceAlert(const QString& alert);

private slots:
    void playbackLoop();
    void updatePosition();
    void updateStatistics();
    void checkBufferStatus();
    void handlePlaybackTimer();

private:
    // Initialization and cleanup
    void initializePlayback();
    void cleanupPlayback();
    void setupTimers();
    void cleanupTimers();
    
    // File operations
    bool analyseRecordingFile(const QString& filename);
    bool loadFrameIndex();
    void buildFrameIndex();
    PlaybackFrame readNextFrame();
    bool validateFrame(const PlaybackFrame& frame);
    
    // Playback management
    void startPlaybackLoop();
    void stopPlaybackLoop();
    void processNextFrame();
    void handleEndOfStream();
    void handleLoopBoundary();
    
    // Buffer management
    void fillPlaybackBuffer();
    void maintainBufferLevel();
    void handleBufferUnderrun();
    void handleBufferOverrun();
    
    // Quality management
    void analyzeFrameQuality(PlaybackFrame& frame);
    void updateQualityStatistics(const PlaybackFrame& frame);
    void handleQualityDegradation(double quality);
    
    // Performance monitoring
    void updatePerformanceMetrics();
    void monitorResourceUsage();
    void optimizePlaybackPerformance();
    
    // Core components
    std::unique_ptr<QFile> m_recordingFile;
    std::unique_ptr<QDataStream> m_dataStream;
    std::unique_ptr<QTimer> m_playbackTimer;
    std::unique_ptr<QTimer> m_positionTimer;
    std::unique_ptr<QTimer> m_statisticsTimer;
    std::unique_ptr<QTimer> m_bufferTimer;
    
    // Playback engines
    std::unique_ptr<TimingEngine> m_timingEngine;
    std::unique_ptr<FrameBuffer> m_frameBuffer;
    std::unique_ptr<PlaybackController> m_controller;
    
    // Configuration and state
    PlaybackConfig m_config;
    PlaybackStatistics m_statistics;
    QString m_currentRecording;
    eti::RecordingFormat m_recordingFormat = eti::RecordingFormat::ETI_NI;
    
    // Frame indexing
    struct FrameIndex {
        qint64 file_position;
        qint64 frame_number;
        QDateTime timestamp;
        size_t frame_size;
        uint32_t checksum;
    };
    
    std::vector<FrameIndex> m_frameIndex;
    qint64 m_currentFilePosition = 0;
    bool m_indexBuilt = false;
    
    // State management
    mutable QMutex m_playbackMutex;
    std::atomic<bool> m_recordingLoaded{false};
    std::atomic<bool> m_playbackActive{false};
    std::atomic<bool> m_performanceMonitoring{true};
    
    // Callbacks
    FrameCallback m_frameCallback;
    PositionCallback m_positionCallback;
    StateCallback m_stateCallback;
    ErrorCallback m_errorCallback;
    mutable QMutex m_callbackMutex;
    
    // Performance tracking
    std::chrono::steady_clock::time_point m_playbackStartTime;
    std::chrono::steady_clock::time_point m_lastFrameTime;
    std::vector<double> m_performanceHistory;
    
    // Error handling
    QStringList m_playbackErrors;
    mutable QMutex m_errorMutex;
    static constexpr size_t MAX_ERROR_HISTORY = 100;
    
    // Timing constants
    static constexpr std::chrono::milliseconds PLAYBACK_TIMER_INTERVAL{10};
    static constexpr std::chrono::milliseconds POSITION_UPDATE_INTERVAL{100};
    static constexpr std::chrono::milliseconds STATISTICS_UPDATE_INTERVAL{1000};
    static constexpr std::chrono::milliseconds BUFFER_CHECK_INTERVAL{500};
};

/**
 * @brief Factory for creating specialized playback engines
 */
class StreamPlaybackFactory {
public:
    /**
     * @brief Create real-time playback engine for live monitoring
     */
    static std::unique_ptr<StreamPlayback> createRealTimePlayer();
    
    /**
     * @brief Create analysis playback engine for detailed frame analysis
     */
    static std::unique_ptr<StreamPlayback> createAnalysisPlayer();
    
    /**
     * @brief Create fast scan player for quick content review
     */
    static std::unique_ptr<StreamPlayback> createFastScanPlayer();
    
    /**
     * @brief Create frame-accurate player for precise navigation
     */
    static std::unique_ptr<StreamPlayback> createFrameAccuratePlayer();
    
    /**
     * @brief Create broadcast simulation player
     */
    static std::unique_ptr<StreamPlayback> createBroadcastSimulationPlayer();
    
    /**
     * @brief Create custom player with specific configuration
     */
    static std::unique_ptr<StreamPlayback> createCustomPlayer(const PlaybackConfig& config);
};

} // namespace eti_network

#endif // STREAM_PLAYBACK_H