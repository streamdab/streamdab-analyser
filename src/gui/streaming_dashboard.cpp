/**
 * @file streaming_dashboard.cpp
 * @brief Implementation of Live Streaming Dashboard
 * 
 * @author StreamDAB Development Team
 * @date 2025
 * @copyright Copyright (c) 2025 StreamDAB Technologies
 */

#include "streaming_dashboard.h"
#include "broadcast_theme.h"

#include <QApplication>
#include <QPalette>
#include <QSplitter>
#include <QHeaderView>
#include <QMessageBox>
#include <QToolTip>
#include <cmath>

namespace streamdab::ui {

// StatusIndicatorWidget implementation
StatusIndicatorWidget::StatusIndicatorWidget(QWidget* parent)
    : QWidget(parent)
    , blink_timer_(new QTimer(this))
{
    setFixedSize(120, 24);
    
    blink_timer_->setInterval(500); // 500ms blink
    connect(blink_timer_, &QTimer::timeout, this, &StatusIndicatorWidget::onBlinkTimer);
}

void StatusIndicatorWidget::setStatus(ConnectionStatus status) {
    if (status_ != status) {
        status_ = status;
        update();
    }
}

void StatusIndicatorWidget::setQuality(QualityIndicator quality) {
    if (quality_ != quality) {
        quality_ = quality;
        update();
    }
}

void StatusIndicatorWidget::setText(const QString& text) {
    if (text_ != text) {
        text_ = text;
        update();
    }
}

void StatusIndicatorWidget::setBlinking(bool enabled) {
    if (blinking_ != enabled) {
        blinking_ = enabled;
        if (enabled) {
            blink_timer_->start();
        } else {
            blink_timer_->stop();
            blink_state_ = false;
        }
        update();
    }
}

void StatusIndicatorWidget::paintEvent(QPaintEvent* event) {
    Q_UNUSED(event)
    
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    
    QRect rect = this->rect().adjusted(2, 2, -2, -2);
    
    // Get colors based on status and quality
    QColor bg_color = getStatusColor();
    if (quality_ != QualityIndicator::UNKNOWN) {
        bg_color = getQualityColor();
    }
    
    // Apply blinking effect
    if (blinking_ && !blink_state_) {
        bg_color = bg_color.lighter(150);
    }
    
    // Draw background
    painter.setBrush(bg_color);
    painter.setPen(QPen(bg_color.darker(120), 1));
    painter.drawRoundedRect(rect, 4, 4);
    
    // Draw text
    painter.setPen(QPen(Qt::white));
    painter.setFont(QFont("Arial", 9, QFont::Bold));
    painter.drawText(rect, Qt::AlignCenter, text_);
}

void StatusIndicatorWidget::onBlinkTimer() {
    blink_state_ = !blink_state_;
    update();
}

QColor StatusIndicatorWidget::getStatusColor() const {
    switch (status_) {
        case ConnectionStatus::CONNECTED:
        case ConnectionStatus::STREAMING:
            return QColor(0, 150, 0); // Green
        case ConnectionStatus::CONNECTING:
        case ConnectionStatus::RECONNECTING:
            return QColor(255, 165, 0); // Orange
        case ConnectionStatus::ERROR:
            return QColor(220, 20, 20); // Red
        case ConnectionStatus::DISCONNECTED:
        default:
            return QColor(128, 128, 128); // Gray
    }
}

QColor StatusIndicatorWidget::getQualityColor() const {
    switch (quality_) {
        case QualityIndicator::EXCELLENT:
            return QColor(0, 180, 0); // Bright Green
        case QualityIndicator::GOOD:
            return QColor(100, 200, 0); // Light Green
        case QualityIndicator::ACCEPTABLE:
            return QColor(255, 200, 0); // Yellow
        case QualityIndicator::POOR:
            return QColor(255, 100, 0); // Orange
        case QualityIndicator::CRITICAL:
            return QColor(220, 20, 20); // Red
        case QualityIndicator::UNKNOWN:
        default:
            return QColor(128, 128, 128); // Gray
    }
}

// RealTimeChartWidget implementation
RealTimeChartWidget::RealTimeChartWidget(QWidget* parent)
    : QWidget(parent)
{
    setMinimumSize(200, 120);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
}

void RealTimeChartWidget::addDataPoint(double value) {
    data_points_.push_back(value);
    
    // Maintain max data points
    while (static_cast<int>(data_points_.size()) > max_data_points_) {
        data_points_.pop_front();
    }
    
    // Auto-adjust range if needed
    if (value > max_value_) {
        max_value_ = value * 1.1; // Add 10% headroom
    }
    
    update();
}

void RealTimeChartWidget::setMaxDataPoints(int max_points) {
    max_data_points_ = max_points;
    while (static_cast<int>(data_points_.size()) > max_data_points_) {
        data_points_.pop_front();
    }
}

void RealTimeChartWidget::setValueRange(double min_val, double max_val) {
    min_value_ = min_val;
    max_value_ = max_val;
    update();
}

void RealTimeChartWidget::setTitle(const QString& title) {
    title_ = title;
    update();
}

void RealTimeChartWidget::setUnit(const QString& unit) {
    unit_ = unit;
    update();
}

void RealTimeChartWidget::setColor(const QColor& color) {
    chart_color_ = color;
    update();
}

void RealTimeChartWidget::clear() {
    data_points_.clear();
    update();
}

void RealTimeChartWidget::paintEvent(QPaintEvent* event) {
    Q_UNUSED(event)
    
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    
    QRect widget_rect = rect();
    QRect chart_rect = widget_rect.adjusted(40, 20, -10, -30);
    
    // Draw background
    painter.fillRect(widget_rect, QColor(250, 250, 250));
    painter.setPen(QPen(Qt::black, 1));
    painter.drawRect(chart_rect);
    
    if (data_points_.empty()) {
        painter.drawText(chart_rect, Qt::AlignCenter, "No Data");
        return;
    }
    
    // Draw grid
    drawGrid(painter, chart_rect);
    
    // Draw chart
    drawChart(painter, chart_rect);
    
    // Draw labels
    drawLabels(painter, widget_rect);
}

void RealTimeChartWidget::drawGrid(QPainter& painter, const QRect& chart_rect) {
    painter.setPen(QPen(QColor(200, 200, 200), 1, Qt::DotLine));
    
    // Horizontal grid lines
    for (int i = 1; i < 5; ++i) {
        int y = chart_rect.top() + (chart_rect.height() * i) / 5;
        painter.drawLine(chart_rect.left(), y, chart_rect.right(), y);
    }
    
    // Vertical grid lines
    for (int i = 1; i < 10; ++i) {
        int x = chart_rect.left() + (chart_rect.width() * i) / 10;
        painter.drawLine(x, chart_rect.top(), x, chart_rect.bottom());
    }
}

void RealTimeChartWidget::drawChart(QPainter& painter, const QRect& chart_rect) {
    if (data_points_.size() < 2) return;
    
    painter.setPen(QPen(chart_color_, 2));
    
    double value_range = max_value_ - min_value_;
    if (value_range <= 0) value_range = 1.0;
    
    QPointF prev_point;
    bool first_point = true;
    
    for (size_t i = 0; i < data_points_.size(); ++i) {
        double x_ratio = static_cast<double>(i) / (max_data_points_ - 1);
        double y_ratio = (data_points_[i] - min_value_) / value_range;
        
        QPointF point(
            chart_rect.left() + x_ratio * chart_rect.width(),
            chart_rect.bottom() - y_ratio * chart_rect.height()
        );
        
        if (!first_point) {
            painter.drawLine(prev_point, point);
        }
        
        prev_point = point;
        first_point = false;
    }
    
    // Draw current value point
    if (!data_points_.empty()) {
        painter.setBrush(chart_color_);
        painter.drawEllipse(prev_point, 3, 3);
    }
}

void RealTimeChartWidget::drawLabels(QPainter& painter, const QRect& widget_rect) {
    painter.setPen(Qt::black);
    painter.setFont(QFont("Arial", 8));
    
    // Title
    if (!title_.isEmpty()) {
        painter.setFont(QFont("Arial", 9, QFont::Bold));
        painter.drawText(widget_rect.adjusted(0, 0, 0, -widget_rect.height() + 15), 
                        Qt::AlignCenter, title_);
        painter.setFont(QFont("Arial", 8));
    }
    
    // Y-axis labels
    for (int i = 0; i <= 5; ++i) {
        double value = min_value_ + (max_value_ - min_value_) * (5 - i) / 5.0;
        QString label = QString::number(value, 'f', 1);
        
        int y = widget_rect.top() + 20 + (widget_rect.height() - 50) * i / 5;
        painter.drawText(5, y + 4, label);
    }
    
    // Current value and unit
    if (!data_points_.empty()) {
        QString current_text = QString("%1 %2")
                              .arg(data_points_.back(), 0, 'f', 1)
                              .arg(unit_);
        painter.setFont(QFont("Arial", 8, QFont::Bold));
        painter.drawText(widget_rect.adjusted(0, widget_rect.height() - 15, 0, 0),
                        Qt::AlignCenter, current_text);
    }
}

// StreamingDashboard implementation
StreamingDashboard::StreamingDashboard(QWidget* parent)
    : QWidget(parent)
    , config_dialog_(std::make_unique<ConnectionConfigDialog>(this))
    , update_timer_(new QTimer(this))
{
    setupUI();
    connectSignals();
    applyProfessionalStyling();
    
    // Initialize update timer
    update_timer_->setInterval(DEFAULT_UPDATE_INTERVAL);
    connect(update_timer_, &QTimer::timeout, this, &StreamingDashboard::onUpdateTimer);
    
    // Initialize metrics
    current_metrics_ = DashboardMetrics{};
    
    // Start with disconnected state
    updateConnectionStatus(ConnectionStatus::DISCONNECTED);
}

StreamingDashboard::~StreamingDashboard() {
    if (update_timer_->isActive()) {
        update_timer_->stop();
    }
}

bool StreamingDashboard::initialize(std::shared_ptr<eti::streaming::StreamingEngine> streaming_engine,
                                   std::shared_ptr<eti::controller::ModernETIController> eti_controller) {
    if (!streaming_engine || !eti_controller) {
        return false;
    }
    
    streaming_engine_ = streaming_engine;
    eti_controller_ = eti_controller;
    
    // Connect streaming engine signals
    connect(streaming_engine_.get(), &eti::streaming::StreamingEngine::streamingStateChanged,
            this, &StreamingDashboard::onStreamingStateChanged);
    connect(streaming_engine_.get(), &eti::streaming::StreamingEngine::frameProcessed,
            this, &StreamingDashboard::onFrameProcessed);
    connect(streaming_engine_.get(), &eti::streaming::StreamingEngine::bufferStatusChanged,
            this, &StreamingDashboard::onBufferStatusChanged);
    connect(streaming_engine_.get(), &eti::streaming::StreamingEngine::qualityLevelChanged,
            this, &StreamingDashboard::onQualityLevelChanged);
    connect(streaming_engine_.get(), &eti::streaming::StreamingEngine::qualityAlert,
            this, &StreamingDashboard::onQualityAlert);
    
    // Connect ETI controller signals
    connect(eti_controller_.get(), &eti::controller::ModernETIController::connectionStatusChanged,
            this, &StreamingDashboard::onConnectionStatusChanged);
    connect(eti_controller_.get(), &eti::controller::ModernETIController::statisticsUpdated,
            this, &StreamingDashboard::onStatisticsUpdated);
    connect(eti_controller_.get(), &eti::controller::ModernETIController::performanceTargetStatus,
            this, &StreamingDashboard::onPerformanceTargetStatus);
    
    initialized_ = true;
    
    // Start auto-refresh if enabled
    if (auto_refresh_enabled_) {
        update_timer_->start();
    }
    
    return true;
}

void StreamingDashboard::setupUI() {
    main_layout_ = new QVBoxLayout(this);
    main_layout_->setSpacing(8);
    main_layout_->setContentsMargins(8, 8, 8, 8);
    
    // Create tab widget for different metric categories
    metrics_tabs_ = new QTabWidget(this);
    
    // Overview tab with main metrics
    auto* overview_widget = new QWidget();
    auto* overview_layout = new QGridLayout(overview_widget);
    overview_layout->setSpacing(8);
    
    // Add metric panels to overview
    overview_layout->addWidget(createConnectionStatusPanel(), 0, 0);
    overview_layout->addWidget(createStreamingMetricsPanel(), 0, 1);
    overview_layout->addWidget(createBufferStatusPanel(), 1, 0);
    overview_layout->addWidget(createQualityMonitoringPanel(), 1, 1);
    
    metrics_tabs_->addTab(overview_widget, QIcon(":/icons/overview.png"), "Overview");
    
    // Charts tab
    auto* charts_widget = createPerformanceChartsPanel();
    metrics_tabs_->addTab(charts_widget, QIcon(":/icons/charts.png"), "Performance Charts");
    
    // Add control buttons panel
    auto* control_panel = createControlButtonsPanel();
    
    main_layout_->addWidget(metrics_tabs_);
    main_layout_->addWidget(control_panel);
}

QWidget* StreamingDashboard::createConnectionStatusPanel() {
    connection_group_ = new QGroupBox("Connection Status");
    auto* layout = new QVBoxLayout(connection_group_);
    
    // Status indicator
    connection_indicator_ = new StatusIndicatorWidget();
    connection_indicator_->setText("DISCONNECTED");
    connection_indicator_->setStatus(ConnectionStatus::DISCONNECTED);
    
    // Connection info
    endpoint_label_ = new QLabel("Endpoint: Not connected");
    endpoint_label_->setWordWrap(true);
    
    uptime_label_ = new QLabel("Uptime: --");
    
    status_text_label_ = new QLabel("Status: Disconnected");
    status_text_label_->setStyleSheet("color: #666666; font-style: italic;");
    
    layout->addWidget(connection_indicator_);
    layout->addWidget(endpoint_label_);
    layout->addWidget(uptime_label_);
    layout->addWidget(status_text_label_);
    layout->addStretch();
    
    return connection_group_;
}

QWidget* StreamingDashboard::createStreamingMetricsPanel() {
    streaming_group_ = new QGroupBox("Streaming Metrics");
    auto* layout = new QFormLayout(streaming_group_);
    
    // FPS display
    fps_display_ = new QLCDNumber(6);
    fps_display_->setSegmentStyle(QLCDNumber::Flat);
    fps_display_->setDigitCount(6);
    fps_display_->display("0.0");
    
    // Total frames
    total_frames_display_ = new QLCDNumber(8);
    total_frames_display_->setSegmentStyle(QLCDNumber::Flat);
    total_frames_display_->setDigitCount(8);
    total_frames_display_->display(0);
    
    // Error rate
    error_rate_label_ = new QLabel("0.0%");
    error_rate_label_->setAlignment(Qt::AlignRight);
    
    // Throughput bar
    throughput_bar_ = new QProgressBar();
    throughput_bar_->setRange(0, 100);
    throughput_bar_->setValue(0);
    throughput_bar_->setFormat("%v Mbps");
    
    layout->addRow("Current FPS:", fps_display_);
    layout->addRow("Total Frames:", total_frames_display_);
    layout->addRow("Error Rate:", error_rate_label_);
    layout->addRow("Throughput:", throughput_bar_);
    
    return streaming_group_;
}

QWidget* StreamingDashboard::createBufferStatusPanel() {
    buffer_group_ = new QGroupBox("Buffer Status");
    auto* layout = new QFormLayout(buffer_group_);
    
    // Buffer utilization
    buffer_utilization_bar_ = new QProgressBar();
    buffer_utilization_bar_->setRange(0, 100);
    buffer_utilization_bar_->setValue(0);
    buffer_utilization_bar_->setFormat("%p%");
    
    // Buffer size info
    buffer_size_label_ = new QLabel("0 / 1000 frames");
    buffer_size_label_->setAlignment(Qt::AlignRight);
    
    // Buffer latency
    buffer_latency_label_ = new QLabel("0 ms");
    buffer_latency_label_->setAlignment(Qt::AlignRight);
    
    // Buffer overflows
    buffer_overflows_label_ = new QLabel("0");
    buffer_overflows_label_->setAlignment(Qt::AlignRight);
    
    layout->addRow("Utilization:", buffer_utilization_bar_);
    layout->addRow("Size:", buffer_size_label_);
    layout->addRow("Latency:", buffer_latency_label_);
    layout->addRow("Overflows:", buffer_overflows_label_);
    
    return buffer_group_;
}

QWidget* StreamingDashboard::createQualityMonitoringPanel() {
    quality_group_ = new QGroupBox("Quality Monitoring");
    auto* layout = new QFormLayout(quality_group_);
    
    // Quality indicator
    quality_indicator_ = new StatusIndicatorWidget();
    quality_indicator_->setText("UNKNOWN");
    quality_indicator_->setQuality(QualityIndicator::UNKNOWN);
    
    // Signal quality bar
    signal_quality_bar_ = new QProgressBar();
    signal_quality_bar_->setRange(0, 100);
    signal_quality_bar_->setValue(0);
    signal_quality_bar_->setFormat("%p%");
    
    // ETSI compliance bar
    etsi_compliance_bar_ = new QProgressBar();
    etsi_compliance_bar_->setRange(0, 100);
    etsi_compliance_bar_->setValue(0);
    etsi_compliance_bar_->setFormat("%p%");
    
    // Sync accuracy
    sync_accuracy_label_ = new QLabel("-- μs");
    sync_accuracy_label_->setAlignment(Qt::AlignRight);
    
    layout->addRow("Overall Quality:", quality_indicator_);
    layout->addRow("Signal Quality:", signal_quality_bar_);
    layout->addRow("ETSI Compliance:", etsi_compliance_bar_);
    layout->addRow("Sync Accuracy:", sync_accuracy_label_);
    
    return quality_group_;
}

QWidget* StreamingDashboard::createPerformanceChartsPanel() {
    charts_group_ = new QGroupBox("Performance Charts");
    auto* layout = new QGridLayout(charts_group_);
    
    // Create charts
    fps_chart_ = new RealTimeChartWidget();
    fps_chart_->setTitle("Frame Rate");
    fps_chart_->setUnit("FPS");
    fps_chart_->setColor(QColor(0, 120, 255));
    fps_chart_->setValueRange(0, 1200);
    
    latency_chart_ = new RealTimeChartWidget();
    latency_chart_->setTitle("Processing Latency");
    latency_chart_->setUnit("μs");
    latency_chart_->setColor(QColor(255, 120, 0));
    latency_chart_->setValueRange(0, 5000);
    
    quality_chart_ = new RealTimeChartWidget();
    quality_chart_->setTitle("Signal Quality");
    quality_chart_->setUnit("%");
    quality_chart_->setColor(QColor(0, 180, 0));
    quality_chart_->setValueRange(0, 100);
    
    buffer_chart_ = new RealTimeChartWidget();
    buffer_chart_->setTitle("Buffer Utilization");
    buffer_chart_->setUnit("%");
    buffer_chart_->setColor(QColor(180, 0, 180));
    buffer_chart_->setValueRange(0, 100);
    
    // Arrange charts in 2x2 grid
    layout->addWidget(fps_chart_, 0, 0);
    layout->addWidget(latency_chart_, 0, 1);
    layout->addWidget(quality_chart_, 1, 0);
    layout->addWidget(buffer_chart_, 1, 1);
    
    return charts_group_;
}

QWidget* StreamingDashboard::createControlButtonsPanel() {
    control_group_ = new QGroupBox("Stream Control");
    auto* layout = new QHBoxLayout(control_group_);
    
    configure_btn_ = new QPushButton(QIcon(":/icons/configure.png"), "Configure");
    configure_btn_->setToolTip("Configure connection settings");
    
    start_btn_ = new QPushButton(QIcon(":/icons/play.png"), "Start");
    start_btn_->setToolTip("Start streaming connection");
    
    stop_btn_ = new QPushButton(QIcon(":/icons/stop.png"), "Stop");
    stop_btn_->setToolTip("Stop streaming connection");
    stop_btn_->setEnabled(false);
    
    reconnect_btn_ = new QPushButton(QIcon(":/icons/reconnect.png"), "Reconnect");
    reconnect_btn_->setToolTip("Reconnect to streaming source");
    reconnect_btn_->setEnabled(false);
    
    reset_stats_btn_ = new QPushButton(QIcon(":/icons/reset.png"), "Reset Stats");
    reset_stats_btn_->setToolTip("Reset all statistics and charts");
    
    layout->addWidget(configure_btn_);
    layout->addStretch();
    layout->addWidget(start_btn_);
    layout->addWidget(stop_btn_);
    layout->addWidget(reconnect_btn_);
    layout->addStretch();
    layout->addWidget(reset_stats_btn_);
    
    return control_group_;
}

void StreamingDashboard::connectSignals() {
    // Control button signals
    connect(configure_btn_, &QPushButton::clicked,
            this, &StreamingDashboard::showConnectionConfig);
    connect(start_btn_, &QPushButton::clicked,
            this, &StreamingDashboard::startStreaming);
    connect(stop_btn_, &QPushButton::clicked,
            this, &StreamingDashboard::stopStreaming);
    connect(reconnect_btn_, &QPushButton::clicked,
            this, &StreamingDashboard::reconnectStream);
    connect(reset_stats_btn_, &QPushButton::clicked,
            this, &StreamingDashboard::resetStatistics);
    
    // Configuration dialog signals
    connect(config_dialog_.get(), &ConnectionConfigDialog::configurationAccepted,
            this, [this](const ConnectionConfig& config) {
                current_metrics_.endpoint_url = config.full_url;
                updateConnectionInfo(config.full_url);
            });
}

void StreamingDashboard::applyProfessionalStyling() {
    // Apply broadcast theme
    BroadcastTheme::applyTheme(this);
    
    // Custom styling for dashboard components
    setStyleSheet(styleSheet() + R"(
        QGroupBox {
            font-weight: bold;
            border: 2px solid #cccccc;
            border-radius: 6px;
            margin-top: 8px;
            padding-top: 8px;
            background-color: white;
        }
        
        QGroupBox::title {
            subcontrol-origin: margin;
            left: 8px;
            padding: 0 4px 0 4px;
            color: #333333;
        }
        
        QLCDNumber {
            background-color: #000000;
            color: #00ff00;
            border: 1px solid #666666;
            border-radius: 3px;
        }
        
        QProgressBar {
            border: 1px solid #cccccc;
            border-radius: 3px;
            text-align: center;
            background-color: #f0f0f0;
        }
        
        QProgressBar::chunk {
            background-color: #0078d4;
            border-radius: 2px;
        }
        
        QPushButton {
            padding: 6px 12px;
            border: 1px solid #cccccc;
            border-radius: 3px;
            background-color: #f8f8f8;
            font-weight: bold;
        }
        
        QPushButton:hover {
            background-color: #e8e8e8;
        }
        
        QPushButton:pressed {
            background-color: #d8d8d8;
        }
        
        QPushButton:disabled {
            background-color: #f0f0f0;
            color: #999999;
        }
    )");
}

void StreamingDashboard::setAutoRefreshEnabled(bool enabled, int interval_ms) {
    auto_refresh_enabled_ = enabled;
    
    if (enabled && initialized_) {
        update_timer_->setInterval(interval_ms);
        update_timer_->start();
    } else {
        update_timer_->stop();
    }
}

void StreamingDashboard::setDarkTheme(bool dark) {
    dark_theme_ = dark;
    applyProfessionalStyling(); // Reapply styling with theme change
}

void StreamingDashboard::resetStatistics() {
    current_metrics_ = DashboardMetrics{};
    current_metrics_.connection_status = getConnectionStatusFromEngine();
    
    // Clear charts
    if (fps_chart_) fps_chart_->clear();
    if (latency_chart_) latency_chart_->clear();
    if (quality_chart_) quality_chart_->clear();
    if (buffer_chart_) buffer_chart_->clear();
    
    // Reset tracking variables
    last_frame_count_ = 0;
    recent_fps_values_.clear();
    
    // Reset timers
    uptime_timer_.restart();
    connection_start_time_ = std::chrono::system_clock::now();
    
    updateAllDisplays();
}

void StreamingDashboard::updateConnectionStatus(ConnectionStatus status) {
    if (current_metrics_.connection_status != status) {
        current_metrics_.connection_status = status;
        
        // Update connection start time for uptime calculation
        if (status == ConnectionStatus::CONNECTED || status == ConnectionStatus::STREAMING) {
            if (!uptime_timer_.isValid()) {
                uptime_timer_.start();
                connection_start_time_ = std::chrono::system_clock::now();
            }
        } else if (status == ConnectionStatus::DISCONNECTED) {
            uptime_timer_.invalidate();
        }
        
        updateConnectionIndicators();
        emit metricsChanged(current_metrics_);
    }
}

void StreamingDashboard::updateQualityMetrics(const eti::streaming::QualityMetrics& metrics) {
    current_metrics_.signal_quality = metrics.getOverallQuality();
    current_metrics_.etsi_compliance = metrics.etsi_compliance_score;
    current_metrics_.quality_level = convertQualityLevel(metrics.current_level);
    current_metrics_.sync_accuracy = std::chrono::duration_cast<std::chrono::microseconds>(
        std::chrono::nanoseconds(static_cast<long long>(metrics.sync_accuracy.count() * 1000)));
    current_metrics_.error_rate = metrics.current_error_rate;
    
    updateQualityDisplays();
    checkQualityAlerts();
    
    emit metricsChanged(current_metrics_);
}

void StreamingDashboard::updateBufferStatus(const eti::streaming::BufferStatus& status) {
    current_metrics_.buffer_size = static_cast<uint32_t>(status.current_size);
    current_metrics_.buffer_capacity = static_cast<uint32_t>(status.max_size);
    current_metrics_.buffer_utilization = status.utilization_percent;
    current_metrics_.buffer_latency = status.avg_latency;
    current_metrics_.buffer_overflows = static_cast<uint32_t>(status.overflow_count);
    current_metrics_.buffer_underflows = static_cast<uint32_t>(status.underflow_count);
    
    updateBufferDisplays();
    
    emit metricsChanged(current_metrics_);
}

void StreamingDashboard::updatePerformanceStats(const eti::streaming::StreamingEngine::PerformanceStats& stats) {
    current_metrics_.current_fps = stats.current_fps;
    current_metrics_.average_fps = stats.average_fps;
    current_metrics_.total_frames = stats.total_frames_processed;
    current_metrics_.frames_with_errors = stats.frames_with_errors;
    current_metrics_.processing_latency = stats.avg_processing_time;
    current_metrics_.cpu_usage = stats.cpu_usage_percent;
    current_metrics_.memory_usage = stats.memory_usage_mb;
    
    updateStreamingDisplays();
    updatePerformanceCharts();
    
    emit metricsChanged(current_metrics_);
}

void StreamingDashboard::updateConnectionInfo(const QString& endpoint) {
    current_metrics_.endpoint_url = endpoint;
    updateConnectionIndicators();
}

void StreamingDashboard::showConnectionConfig() {
    config_dialog_->show();
    config_dialog_->raise();
    config_dialog_->activateWindow();
}

void StreamingDashboard::startStreaming() {
    if (!initialized_) {
        QMessageBox::warning(this, "Dashboard Not Initialized", 
                           "Dashboard must be initialized before starting streaming.");
        return;
    }
    
    emit streamingStartRequested(current_metrics_.endpoint_url);
}

void StreamingDashboard::stopStreaming() {
    emit streamingStopRequested();
}

void StreamingDashboard::reconnectStream() {
    emit reconnectionRequested();
}

void StreamingDashboard::refreshDisplay() {
    updateAllDisplays();
}

void StreamingDashboard::onUpdateTimer() {
    if (!initialized_) {
        return;
    }
    
    // Update uptime
    if (uptime_timer_.isValid()) {
        current_metrics_.connection_uptime = std::chrono::milliseconds(uptime_timer_.elapsed());
    }
    
    // Update FPS calculation
    auto now = std::chrono::system_clock::now();
    if (last_fps_update_ != std::chrono::system_clock::time_point{}) {
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - last_fps_update_);
        if (elapsed.count() > 0) {
            uint64_t frame_diff = current_metrics_.total_frames - last_frame_count_;
            double fps = (frame_diff * 1000.0) / elapsed.count();
            
            recent_fps_values_.push_back(fps);
            if (recent_fps_values_.size() > FPS_HISTORY_SIZE) {
                recent_fps_values_.pop_front();
            }
            
            current_metrics_.current_fps = dashboard_utils::calculateMovingAverage(recent_fps_values_, FPS_HISTORY_SIZE);
        }
    }
    
    last_fps_update_ = now;
    last_frame_count_ = current_metrics_.total_frames;
    
    updateAllDisplays();
}

void StreamingDashboard::onStreamingStateChanged(bool streaming) {
    ConnectionStatus new_status = streaming ? ConnectionStatus::STREAMING : ConnectionStatus::CONNECTED;
    updateConnectionStatus(new_status);
    
    // Update control buttons
    start_btn_->setEnabled(!streaming);
    stop_btn_->setEnabled(streaming);
    reconnect_btn_->setEnabled(streaming);
}

void StreamingDashboard::onFrameProcessed(uint32_t frame_number, const eti::EtiFrame& frame,
                                         std::chrono::microseconds processing_time) {
    Q_UNUSED(frame_number)
    Q_UNUSED(frame)
    
    current_metrics_.total_frames++;
    current_metrics_.processing_latency = processing_time;
    
    // Update performance charts
    updatePerformanceCharts();
}

void StreamingDashboard::onBufferStatusChanged(const eti::streaming::BufferStatus& status) {
    updateBufferStatus(status);
}

void StreamingDashboard::onQualityLevelChanged(eti::streaming::QualityLevel level,
                                              const eti::streaming::QualityMetrics& metrics) {
    updateQualityMetrics(metrics);
}

void StreamingDashboard::onQualityAlert(eti::streaming::QualityLevel level, const QString& message) {
    QualityIndicator indicator = convertQualityLevel(level);
    emit qualityAlert(indicator, message);
    
    // Show visual alert
    if (quality_indicator_) {
        quality_indicator_->setBlinking(true);
        QTimer::singleShot(3000, [this]() {
            if (quality_indicator_) {
                quality_indicator_->setBlinking(false);
            }
        });
    }
}

void StreamingDashboard::onConnectionStatusChanged(const eti::network::ConnectionStatus& status) {
    ConnectionStatus dashboard_status = status.isConnected() ? 
                                       ConnectionStatus::CONNECTED : 
                                       ConnectionStatus::DISCONNECTED;
    updateConnectionStatus(dashboard_status);
}

void StreamingDashboard::onStatisticsUpdated(const eti::controller::ProcessingStatistics& stats) {
    current_metrics_.total_frames = stats.total_frames_processed;
    current_metrics_.frames_with_errors = stats.frames_with_errors;
    current_metrics_.current_fps = stats.current_fps;
    current_metrics_.average_fps = stats.average_fps;
    current_metrics_.signal_quality = stats.overall_signal_quality;
    current_metrics_.etsi_compliance = stats.etsi_compliance_score;
    
    updateStreamingDisplays();
    updateQualityDisplays();
}

void StreamingDashboard::onPerformanceTargetStatus(bool target_met, double current_fps, double target_fps) {
    Q_UNUSED(target_met)
    Q_UNUSED(target_fps)
    
    current_metrics_.current_fps = current_fps;
    updateStreamingDisplays();
}

void StreamingDashboard::updateAllDisplays() {
    updateConnectionIndicators();
    updateStreamingDisplays();
    updateBufferDisplays();
    updateQualityDisplays();
    updatePerformanceCharts();
}

void StreamingDashboard::updateConnectionIndicators() {
    if (!connection_indicator_ || !endpoint_label_ || !uptime_label_ || !status_text_label_) {
        return;
    }
    
    // Update status indicator
    connection_indicator_->setStatus(current_metrics_.connection_status);
    connection_indicator_->setText(dashboard_utils::connectionStatusToString(current_metrics_.connection_status));
    
    // Update endpoint label
    if (current_metrics_.endpoint_url.isEmpty()) {
        endpoint_label_->setText("Endpoint: Not configured");
    } else {
        endpoint_label_->setText(QString("Endpoint: %1").arg(current_metrics_.endpoint_url));
    }
    
    // Update uptime
    uptime_label_->setText(QString("Uptime: %1").arg(formatUptime(current_metrics_.connection_uptime)));
    
    // Update status text
    status_text_label_->setText(QString("Status: %1").arg(
        dashboard_utils::connectionStatusToString(current_metrics_.connection_status)));
}

void StreamingDashboard::updateStreamingDisplays() {
    if (!fps_display_ || !total_frames_display_ || !error_rate_label_ || !throughput_bar_) {
        return;
    }
    
    // Update FPS display
    fps_display_->display(QString::number(current_metrics_.current_fps, 'f', 1));
    
    // Update total frames
    total_frames_display_->display(static_cast<int>(current_metrics_.total_frames));
    
    // Update error rate
    double error_rate = current_metrics_.total_frames > 0 ? 
                       (static_cast<double>(current_metrics_.frames_with_errors) / current_metrics_.total_frames) * 100.0 : 0.0;
    current_metrics_.error_rate = error_rate;
    error_rate_label_->setText(QString("%1%").arg(error_rate, 0, 'f', 2));
    
    // Update throughput (rough estimate based on FPS)
    double throughput_mbps = (current_metrics_.current_fps * 6144 * 8) / (1024 * 1024); // ETI frame size
    current_metrics_.network_throughput = throughput_mbps;
    throughput_bar_->setValue(static_cast<int>(std::min(throughput_mbps, 100.0)));
    throughput_bar_->setFormat(QString("%1 Mbps").arg(throughput_mbps, 0, 'f', 1));
}

void StreamingDashboard::updateBufferDisplays() {
    if (!buffer_utilization_bar_ || !buffer_size_label_ || !buffer_latency_label_ || !buffer_overflows_label_) {
        return;
    }
    
    // Update buffer utilization
    buffer_utilization_bar_->setValue(static_cast<int>(current_metrics_.buffer_utilization));
    
    // Update buffer size
    buffer_size_label_->setText(QString("%1 / %2 frames")
                               .arg(current_metrics_.buffer_size)
                               .arg(current_metrics_.buffer_capacity));
    
    // Update buffer latency
    buffer_latency_label_->setText(formatLatency(std::chrono::duration_cast<std::chrono::microseconds>(current_metrics_.buffer_latency)));
    
    // Update buffer overflows
    buffer_overflows_label_->setText(QString::number(current_metrics_.buffer_overflows));
}

void StreamingDashboard::updateQualityDisplays() {
    if (!quality_indicator_ || !signal_quality_bar_ || !etsi_compliance_bar_ || !sync_accuracy_label_) {
        return;
    }
    
    // Update quality indicator
    quality_indicator_->setQuality(current_metrics_.quality_level);
    quality_indicator_->setText(dashboard_utils::qualityIndicatorToString(current_metrics_.quality_level));
    
    // Update signal quality bar
    signal_quality_bar_->setValue(static_cast<int>(current_metrics_.signal_quality));
    
    // Update ETSI compliance bar
    etsi_compliance_bar_->setValue(static_cast<int>(current_metrics_.etsi_compliance));
    
    // Update sync accuracy
    sync_accuracy_label_->setText(formatLatency(current_metrics_.sync_accuracy));
}

void StreamingDashboard::updatePerformanceCharts() {
    if (!fps_chart_ || !latency_chart_ || !quality_chart_ || !buffer_chart_) {
        return;
    }
    
    // Add current values to charts
    fps_chart_->addDataPoint(current_metrics_.current_fps);
    latency_chart_->addDataPoint(current_metrics_.processing_latency.count());
    quality_chart_->addDataPoint(current_metrics_.signal_quality);
    buffer_chart_->addDataPoint(current_metrics_.buffer_utilization);
}

void StreamingDashboard::checkQualityAlerts() {
    // Check signal quality threshold
    if (current_metrics_.signal_quality < signal_quality_threshold_) {
        emit qualityAlert(QualityIndicator::POOR, 
                         QString("Signal quality below threshold: %1%").arg(current_metrics_.signal_quality, 0, 'f', 1));
    }
    
    // Check ETSI compliance threshold
    if (current_metrics_.etsi_compliance < etsi_compliance_threshold_) {
        emit qualityAlert(QualityIndicator::POOR,
                         QString("ETSI compliance below threshold: %1%").arg(current_metrics_.etsi_compliance, 0, 'f', 1));
    }
    
    // Check error rate threshold
    if (current_metrics_.error_rate > error_rate_threshold_) {
        emit qualityAlert(QualityIndicator::CRITICAL,
                         QString("Error rate above threshold: %1%").arg(current_metrics_.error_rate, 0, 'f', 1));
    }
    
    // Check buffer utilization threshold
    if (current_metrics_.buffer_utilization > buffer_utilization_threshold_) {
        emit qualityAlert(QualityIndicator::POOR,
                         QString("Buffer utilization high: %1%").arg(current_metrics_.buffer_utilization, 0, 'f', 1));
    }
}

ConnectionStatus StreamingDashboard::getConnectionStatusFromEngine() {
    if (!initialized_ || !streaming_engine_ || !eti_controller_) {
        return ConnectionStatus::DISCONNECTED;
    }
    
    if (streaming_engine_->isStreaming()) {
        return ConnectionStatus::STREAMING;
    } else if (eti_controller_->isProcessing()) {
        return ConnectionStatus::CONNECTED;
    } else {
        return ConnectionStatus::DISCONNECTED;
    }
}

QualityIndicator StreamingDashboard::convertQualityLevel(eti::streaming::QualityLevel level) {
    switch (level) {
        case eti::streaming::QualityLevel::EXCELLENT:
            return QualityIndicator::EXCELLENT;
        case eti::streaming::QualityLevel::GOOD:
            return QualityIndicator::GOOD;
        case eti::streaming::QualityLevel::ACCEPTABLE:
            return QualityIndicator::ACCEPTABLE;
        case eti::streaming::QualityLevel::POOR:
            return QualityIndicator::POOR;
        case eti::streaming::QualityLevel::CRITICAL:
            return QualityIndicator::CRITICAL;
        case eti::streaming::QualityLevel::UNKNOWN:
        default:
            return QualityIndicator::UNKNOWN;
    }
}

QString StreamingDashboard::formatUptime(std::chrono::milliseconds uptime) {
    return dashboard_utils::formatDuration(uptime);
}

QString StreamingDashboard::formatDataRate(double rate_mbps) {
    return QString("%1 Mbps").arg(rate_mbps, 0, 'f', 2);
}

QString StreamingDashboard::formatLatency(std::chrono::microseconds latency) {
    if (latency.count() >= 1000) {
        return QString("%1 ms").arg(latency.count() / 1000.0, 0, 'f', 2);
    } else {
        return QString("%1 μs").arg(latency.count());
    }
}

// Utility functions implementation
namespace dashboard_utils {

QString connectionStatusToString(ConnectionStatus status) {
    switch (status) {
        case ConnectionStatus::DISCONNECTED:
            return "DISCONNECTED";
        case ConnectionStatus::CONNECTING:
            return "CONNECTING";
        case ConnectionStatus::CONNECTED:
            return "CONNECTED";
        case ConnectionStatus::STREAMING:
            return "STREAMING";
        case ConnectionStatus::ERROR:
            return "ERROR";
        case ConnectionStatus::RECONNECTING:
            return "RECONNECTING";
        default:
            return "UNKNOWN";
    }
}

QString qualityIndicatorToString(QualityIndicator indicator) {
    switch (indicator) {
        case QualityIndicator::EXCELLENT:
            return "EXCELLENT";
        case QualityIndicator::GOOD:
            return "GOOD";
        case QualityIndicator::ACCEPTABLE:
            return "ACCEPTABLE";
        case QualityIndicator::POOR:
            return "POOR";
        case QualityIndicator::CRITICAL:
            return "CRITICAL";
        case QualityIndicator::UNKNOWN:
        default:
            return "UNKNOWN";
    }
}

QColor getStatusColor(ConnectionStatus status) {
    switch (status) {
        case ConnectionStatus::CONNECTED:
        case ConnectionStatus::STREAMING:
            return QColor(0, 180, 0); // Green
        case ConnectionStatus::CONNECTING:
        case ConnectionStatus::RECONNECTING:
            return QColor(255, 165, 0); // Orange
        case ConnectionStatus::ERROR:
            return QColor(220, 20, 20); // Red
        case ConnectionStatus::DISCONNECTED:
        default:
            return QColor(128, 128, 128); // Gray
    }
}

QColor getQualityColor(QualityIndicator indicator) {
    switch (indicator) {
        case QualityIndicator::EXCELLENT:
            return QColor(0, 180, 0); // Bright Green
        case QualityIndicator::GOOD:
            return QColor(100, 200, 0); // Light Green
        case QualityIndicator::ACCEPTABLE:
            return QColor(255, 200, 0); // Yellow
        case QualityIndicator::POOR:
            return QColor(255, 100, 0); // Orange
        case QualityIndicator::CRITICAL:
            return QColor(220, 20, 20); // Red
        case QualityIndicator::UNKNOWN:
        default:
            return QColor(128, 128, 128); // Gray
    }
}

QString formatBytes(uint64_t bytes) {
    const char* units[] = {"B", "KB", "MB", "GB", "TB"};
    int unit_index = 0;
    double size = static_cast<double>(bytes);
    
    while (size >= 1024.0 && unit_index < 4) {
        size /= 1024.0;
        unit_index++;
    }
    
    return QString("%1 %2").arg(size, 0, 'f', 2).arg(units[unit_index]);
}

QString formatDuration(std::chrono::milliseconds duration) {
    auto total_seconds = duration.count() / 1000;
    auto hours = total_seconds / 3600;
    auto minutes = (total_seconds % 3600) / 60;
    auto seconds = total_seconds % 60;
    
    if (hours > 0) {
        return QString("%1:%2:%3")
               .arg(hours, 2, 10, QChar('0'))
               .arg(minutes, 2, 10, QChar('0'))
               .arg(seconds, 2, 10, QChar('0'));
    } else {
        return QString("%1:%2")
               .arg(minutes, 2, 10, QChar('0'))
               .arg(seconds, 2, 10, QChar('0'));
    }
}

double calculateMovingAverage(const std::deque<double>& values, size_t window_size) {
    if (values.empty()) {
        return 0.0;
    }
    
    size_t count = std::min(values.size(), window_size);
    double sum = 0.0;
    
    auto it = values.rbegin();
    for (size_t i = 0; i < count; ++i, ++it) {
        sum += *it;
    }
    
    return sum / count;
}

} // namespace dashboard_utils

} // namespace streamdab::ui