/**
 * @file optimized_gui_renderer.hpp  
 * @brief High-Performance GUI Renderer for 60+ FPS Responsiveness
 * 
 * Advanced GUI rendering system with batched updates, dirty region tracking,
 * OpenGL acceleration, and memory-efficient model updates for maintaining
 * smooth 60+ FPS performance during intensive ETI processing.
 * 
 * @author Agent 20 - Performance Optimization Specialist
 * @date 2025-09-26
 */

#pragma once

#include <QObject>
#include <QWidget>
#include <QTimer>
#include <QOpenGLWidget>
#include <QOpenGLFunctions>
#include <QPainter>
#include <QAbstractItemModel>
#include <QStyledItemDelegate>
#include <memory>
#include <atomic>
#include <chrono>
#include <vector>
#include <unordered_map>
#include <mutex>
#include <thread>
#include <condition_variable>

namespace eti::gui {

/**
 * @brief Update priority levels for efficient batching
 */
enum class UpdatePriority {
    CRITICAL = 0,     // Real-time audio levels, error indicators
    HIGH = 1,         // Service status updates, frame counters  
    MEDIUM = 2,       // Service tree updates, statistics
    LOW = 3,          // Background information, labels
    DEFERRED = 4      // Non-critical visual updates
};

/**
 * @brief Dirty region tracking for selective repainting
 */
class DirtyRegionTracker {
public:
    DirtyRegionTracker();
    
    void mark_region_dirty(const QRect& region, UpdatePriority priority);
    void mark_widget_dirty(QWidget* widget, UpdatePriority priority);
    
    std::vector<QRect> get_dirty_regions(UpdatePriority max_priority = UpdatePriority::LOW);
    void clear_dirty_regions();
    
    bool has_dirty_regions() const;
    size_t get_dirty_count() const;
    
private:
    struct DirtyRegion {
        QRect rect;
        UpdatePriority priority;
        std::chrono::high_resolution_clock::time_point timestamp;
    };
    
    mutable std::mutex regions_mutex_;
    std::vector<DirtyRegion> dirty_regions_;
    
    // Optimize overlapping regions
    void consolidate_regions();
};

/**
 * @brief Batched update manager for efficient GUI updates
 */
class BatchedUpdateManager : public QObject {
    Q_OBJECT
    
public:
    explicit BatchedUpdateManager(QObject* parent = nullptr);
    ~BatchedUpdateManager();
    
    /**
     * @brief Schedule widget update with priority
     * @param widget Widget to update
     * @param priority Update priority level
     */
    void schedule_update(QWidget* widget, UpdatePriority priority);
    
    /**
     * @brief Schedule custom update function
     * @param update_function Function to execute
     * @param priority Update priority level
     */
    void schedule_update(std::function<void()> update_function, UpdatePriority priority);
    
    /**
     * @brief Set target frame rate for GUI updates
     * @param fps Target FPS (default: 60)
     */
    void set_target_fps(double fps);
    
    /**
     * @brief Enable/disable adaptive batching
     * @param enabled Adaptive batching based on system load
     */
    void set_adaptive_batching(bool enabled);
    
    /**
     * @brief Get current GUI performance metrics
     */
    struct GuiPerformanceMetrics {
        double current_fps;
        double average_fps;
        double peak_fps;
        std::chrono::microseconds frame_time;
        std::chrono::microseconds peak_frame_time;
        size_t pending_updates;
        size_t processed_updates;
        double cpu_usage_percent;
        size_t memory_usage_bytes;
    };
    
    GuiPerformanceMetrics get_performance_metrics() const;
    
signals:
    void frame_rendered(double fps);
    void performance_warning(const QString& message, double current_fps, double target_fps);
    
private slots:
    void process_update_batch();
    void monitor_performance();
    
private:
    struct PendingUpdate {
        std::function<void()> update_function;
        UpdatePriority priority;
        std::chrono::high_resolution_clock::time_point scheduled_time;
        QWidget* target_widget;
    };
    
    mutable std::mutex updates_mutex_;
    std::vector<std::vector<PendingUpdate>> priority_queues_; // One queue per priority level
    
    std::unique_ptr<QTimer> update_timer_;
    std::unique_ptr<QTimer> performance_timer_;
    
    // Performance tracking
    std::atomic<double> current_fps_{0.0};
    std::atomic<double> target_fps_{60.0};
    std::atomic<bool> adaptive_batching_{true};
    
    std::chrono::high_resolution_clock::time_point last_frame_time_;
    std::vector<std::chrono::microseconds> frame_times_;
    
    // Update processing
    void execute_priority_updates(UpdatePriority priority);
    void adjust_batch_size_adaptive();
    size_t calculate_optimal_batch_size() const;
    
    // Performance monitoring
    void update_performance_statistics();
    bool should_skip_low_priority_updates() const;
};

/**
 * @brief Memory-efficient model for large datasets
 */
class OptimizedTableModel : public QAbstractTableModel {
    Q_OBJECT
    
public:
    explicit OptimizedTableModel(QObject* parent = nullptr);
    
    // QAbstractTableModel interface
    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    int columnCount(const QModelIndex& parent = QModelIndex()) const override;
    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;
    
    /**
     * @brief Add data with batched model updates
     * @param row_data Vector of row data
     * @param batch_size Number of rows to process per batch
     */
    void add_data_batched(const std::vector<QVariantList>& row_data, size_t batch_size = 100);
    
    /**
     * @brief Update specific cells efficiently
     * @param updates Map of (row,col) -> new_value
     */
    void update_cells_batched(const std::unordered_map<std::pair<int, int>, QVariant, 
                                                      std::hash<std::pair<int, int>>>& updates);
    
    /**
     * @brief Enable lazy loading for large datasets
     * @param enabled Enable/disable lazy loading
     * @param page_size Number of items per page
     */
    void set_lazy_loading(bool enabled, size_t page_size = 1000);
    
    /**
     * @brief Clear all data and reset model
     */
    void clear_data();
    
    /**
     * @brief Get model performance statistics
     */
    struct ModelPerformanceStats {
        size_t total_rows;
        size_t total_columns;  
        size_t cached_items;
        size_t memory_usage_bytes;
        std::chrono::microseconds last_update_time;
        size_t pending_updates;
    };
    
    ModelPerformanceStats get_performance_stats() const;
    
private:
    // Data storage with lazy loading
    mutable std::vector<std::vector<QVariant>> cached_data_;
    mutable std::unordered_map<int, std::vector<QVariant>> lazy_loaded_rows_;
    
    // Model configuration
    size_t total_rows_;
    size_t total_columns_;
    bool lazy_loading_enabled_;
    size_t page_size_;
    
    // Performance optimization
    mutable std::mutex data_mutex_;
    std::vector<QString> column_headers_;
    
    // Batch processing
    void process_batch_insert(int start_row, const std::vector<QVariantList>& batch_data);
    void load_row_on_demand(int row) const;
    bool is_row_cached(int row) const;
};

/**
 * @brief OpenGL-accelerated widget renderer
 */
class OpenGLRenderer : public QOpenGLWidget, protected QOpenGLFunctions {
    Q_OBJECT
    
public:
    explicit OpenGLRenderer(QWidget* parent = nullptr);
    ~OpenGLRenderer();
    
    /**
     * @brief Render audio waveform with OpenGL acceleration
     * @param samples Audio samples
     * @param sample_count Number of samples
     * @param color Waveform color
     */
    void render_waveform_gl(const float* samples, size_t sample_count, const QColor& color);
    
    /**
     * @brief Render constellation diagram with GPU acceleration
     * @param i_samples I channel samples
     * @param q_samples Q channel samples  
     * @param sample_count Number of constellation points
     */
    void render_constellation_gl(const float* i_samples, const float* q_samples, size_t sample_count);
    
    /**
     * @brief Render spectrum analyzer with OpenGL
     * @param frequency_bins Frequency bin values
     * @param magnitude_bins Magnitude values (dB)
     * @param bin_count Number of bins
     */
    void render_spectrum_gl(const float* frequency_bins, const float* magnitude_bins, size_t bin_count);
    
    /**
     * @brief Set rendering quality vs performance trade-off
     */
    enum class RenderQuality {
        PERFORMANCE,  // Minimum quality for maximum FPS
        BALANCED,     // Balance quality and performance
        QUALITY       // Maximum quality, may reduce FPS
    };
    
    void set_render_quality(RenderQuality quality);
    
protected:
    void initializeGL() override;
    void resizeGL(int width, int height) override;
    void paintGL() override;
    
private:
    // OpenGL resources
    unsigned int vertex_buffer_object_;
    unsigned int vertex_array_object_;
    unsigned int shader_program_;
    
    // Rendering data
    std::vector<float> vertex_data_;
    std::vector<unsigned int> index_data_;
    RenderQuality render_quality_;
    
    // Performance optimization
    bool use_vertex_buffer_objects_;
    bool use_shader_programs_;
    size_t max_vertices_per_frame_;
    
    // Shader management
    unsigned int compile_shader(const char* source, unsigned int shader_type);
    unsigned int create_shader_program(const char* vertex_source, const char* fragment_source);
    void setup_vertex_attributes();
    
    // Buffer management
    void update_vertex_buffer(const float* data, size_t vertex_count);
    void optimize_buffer_usage();
};

/**
 * @brief High-performance custom painter for 2D graphics
 */
class OptimizedPainter {
public:
    explicit OptimizedPainter(QPainter* painter);
    ~OptimizedPainter();
    
    /**
     * @brief Draw optimized line chart
     * @param points Data points to plot
     * @param color Line color
     * @param line_width Line width in pixels
     */
    void draw_line_chart_optimized(const std::vector<QPointF>& points, 
                                  const QColor& color, 
                                  float line_width = 1.0f);
    
    /**
     * @brief Draw optimized bar chart
     * @param values Bar values
     * @param bar_width Width of each bar
     * @param color Fill color
     */
    void draw_bar_chart_optimized(const std::vector<float>& values,
                                 float bar_width,
                                 const QColor& color);
    
    /**
     * @brief Draw optimized text with caching
     * @param text Text to draw
     * @param position Text position
     * @param font Font to use
     */
    void draw_text_cached(const QString& text, const QPointF& position, const QFont& font);
    
    /**
     * @brief Enable/disable antialiasing based on performance
     * @param enabled Antialiasing enabled
     */
    void set_antialiasing(bool enabled);
    
    /**
     * @brief Set optimization level for drawing operations
     */
    enum class OptimizationLevel {
        SPEED,      // Fastest rendering, minimal quality
        BALANCED,   // Good balance of speed and quality  
        QUALITY     // Best quality, slower rendering
    };
    
    void set_optimization_level(OptimizationLevel level);
    
private:
    QPainter* painter_;
    OptimizationLevel optimization_level_;
    
    // Caching for performance
    mutable std::unordered_map<QString, QPixmap> text_cache_;
    mutable std::unordered_map<QString, QPainterPath> path_cache_;
    
    // Drawing optimizations
    void optimize_pen_settings();
    void optimize_brush_settings();
    QPainterPath get_cached_path(const QString& key, std::function<QPainterPath()> generator);
};

/**
 * @brief Main optimized GUI renderer coordinating all components
 */
class OptimizedGuiRenderer : public QObject {
    Q_OBJECT
    
public:
    explicit OptimizedGuiRenderer(QWidget* main_window, QObject* parent = nullptr);
    ~OptimizedGuiRenderer();
    
    /**
     * @brief Initialize GUI optimization system
     * @return true if initialization successful
     */
    bool initialize();
    
    /**
     * @brief Update audio level displays with batching
     * @param service_levels Map of service_id -> audio_level (0-100)
     */
    void update_audio_levels_batched(const std::unordered_map<uint32_t, float>& service_levels);
    
    /**
     * @brief Update service tree with efficient model operations
     * @param services Vector of service information
     */
    void update_service_tree_optimized(const std::vector<ServiceInfo>& services);
    
    /**
     * @brief Update frame analysis display with memory optimization
     * @param frame_results Vector of frame analysis results
     */
    void update_frame_analysis_optimized(const std::vector<FrameAnalysisResult>& frame_results);
    
    /**
     * @brief Set GUI performance target
     * @param target_fps Target FPS for GUI updates
     */
    void set_performance_target(double target_fps);
    
    /**
     * @brief Enable/disable performance monitoring
     * @param enabled Performance monitoring enabled
     */
    void set_performance_monitoring(bool enabled);
    
    /**
     * @brief Get comprehensive GUI performance metrics
     */
    struct ComprehensiveGuiMetrics {
        BatchedUpdateManager::GuiPerformanceMetrics update_metrics;
        OptimizedTableModel::ModelPerformanceStats model_stats;
        size_t total_widgets;
        size_t active_widgets;
        double memory_usage_mb;
        bool performance_target_met;
    };
    
    ComprehensiveGuiMetrics get_comprehensive_metrics() const;
    
signals:
    void gui_performance_update(double current_fps, double target_fps);
    void gui_performance_warning(const QString& warning, double current_metric);
    void memory_pressure_detected(double memory_usage_mb, double limit_mb);
    
private slots:
    void handle_batch_updates();
    void handle_performance_monitoring();
    void handle_memory_cleanup();
    
private:
    QWidget* main_window_;
    
    // Core optimization components
    std::unique_ptr<BatchedUpdateManager> update_manager_;
    std::unique_ptr<DirtyRegionTracker> dirty_tracker_;
    std::unique_ptr<OptimizedTableModel> table_model_;
    std::unique_ptr<OpenGLRenderer> gl_renderer_;
    
    // Performance monitoring
    std::unique_ptr<QTimer> performance_timer_;
    std::thread performance_monitor_thread_;
    std::atomic<bool> performance_monitoring_active_{false};
    
    // Memory management
    static constexpr double MEMORY_LIMIT_MB = 20.0; // 20MB for GUI components
    std::atomic<double> current_memory_usage_{0.0};
    
    // GUI component optimization
    void optimize_widget_updates();
    void optimize_model_views();
    void optimize_paint_events();
    
    // Memory optimization
    void cleanup_unused_resources();
    void optimize_widget_memory();
    double calculate_gui_memory_usage() const;
    
    // Performance analysis
    void analyze_gui_performance();
    bool meets_performance_targets() const;
    void generate_performance_report() const;
};

} // namespace eti::gui