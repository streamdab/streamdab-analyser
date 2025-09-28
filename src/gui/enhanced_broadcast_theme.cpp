#include "enhanced_broadcast_theme.h"
#include <QApplication>
#include <QWidget>
#include <QPainter>
#include <QPixmap>
#include <QDebug>

EnhancedBroadcastTheme::EnhancedBroadcastTheme(QObject *parent)
    : QObject(parent)
    , m_initialized(false)
{
}

EnhancedBroadcastTheme& EnhancedBroadcastTheme::instance()
{
    static EnhancedBroadcastTheme instance;
    return instance;
}

void EnhancedBroadcastTheme::initialize()
{
    if (m_initialized) {
        return;
    }
    
    // Initialize color mappings and widget styles
    initializeColorMappings();
    createWidgetStyles();
    
    m_initialized = true;
    qDebug() << "Enhanced Broadcast Theme initialized";
}

void EnhancedBroadcastTheme::applyToApplication(QApplication* app)
{
    if (!app) {
        return;
    }
    
    initialize();
    
    // Set application palette
    app->setPalette(createBroadcastPalette());
    
    // Apply application-wide stylesheet
    app->setStyleSheet(getApplicationStyleSheet());
    
    qDebug() << "Broadcast theme applied to application";
}

void EnhancedBroadcastTheme::applyToWidget(QWidget* widget)
{
    if (!widget) {
        return;
    }
    
    initialize();
    
    // Track widget for future theme updates
    if (!m_themedWidgets.contains(widget)) {
        m_themedWidgets.append(widget);
        
        // Connect to widget destruction to clean up tracking
        connect(widget, &QObject::destroyed, this, [this, widget]() {
            m_themedWidgets.removeAll(widget);
        });
    }
    
    // Apply appropriate style based on widget type
    QString className = widget->metaObject()->className();
    
    if (className == "QMainWindow") {
        widget->setStyleSheet(createPanelStyle());
    } else if (className == "QDockWidget") {
        widget->setStyleSheet(createDockWidgetStyle());
    } else if (className == "QTabWidget") {
        widget->setStyleSheet(createTabWidgetStyle());
    } else if (className == "QTableWidget" || className == "QTableView") {
        widget->setStyleSheet(createTableStyle());
    } else if (className == "QTreeWidget" || className == "QTreeView") {
        widget->setStyleSheet(createTreeViewStyle());
    } else {
        // Apply general panel style for other widgets
        widget->setStyleSheet(createPanelStyle());
    }
}

QColor EnhancedBroadcastTheme::getServiceTypeColor(ServiceType type) const
{
    switch (type) {
        case ServiceType::Audio:
            return m_colors.serviceAudio;
        case ServiceType::Data:
            return m_colors.serviceData;
        case ServiceType::Packet:
            return m_colors.servicePacket;
        case ServiceType::Ensemble:
            return m_colors.serviceEnsemble;
        case ServiceType::Unknown:
        default:
            return m_colors.serviceUnknown;
    }
}

QColor EnhancedBroadcastTheme::getQualityLevelColor(QualityLevel level) const
{
    switch (level) {
        case QualityLevel::Excellent:
            return m_colors.qualityExcellent;
        case QualityLevel::Good:
            return m_colors.qualityGood;
        case QualityLevel::Fair:
            return m_colors.qualityFair;
        case QualityLevel::Poor:
            return m_colors.qualityPoor;
        case QualityLevel::Critical:
        default:
            return m_colors.qualityCritical;
    }
}

QColor EnhancedBroadcastTheme::getAlertLevelColor(AlertLevel level) const
{
    switch (level) {
        case AlertLevel::Info:
            return m_colors.alertInfo;
        case AlertLevel::Warning:
            return m_colors.alertWarning;
        case AlertLevel::Error:
            return m_colors.alertError;
        case AlertLevel::Critical:
        default:
            return m_colors.alertCritical;
    }
}

QLinearGradient EnhancedBroadcastTheme::createGradient(const QColor& startColor, 
                                                       const QColor& endColor,
                                                       Qt::Orientation orientation) const
{
    QLinearGradient gradient;
    
    if (orientation == Qt::Vertical) {
        gradient = QLinearGradient(0, 0, 0, 1);
    } else {
        gradient = QLinearGradient(0, 0, 1, 0);
    }
    
    gradient.setColorAt(0.0, startColor);
    gradient.setColorAt(1.0, endColor);
    gradient.setCoordinateMode(QGradient::ObjectBoundingMode);
    
    return gradient;
}

QString EnhancedBroadcastTheme::createButtonStyle(const QColor& baseColor) const
{
    QColor hoverColor = baseColor.lighter(120);
    QColor pressedColor = baseColor.darker(110);
    
    return QString(R"(
        QPushButton {
            background-color: %1;
            color: %2;
            border: 1px solid %3;
            border-radius: 4px;
            padding: 6px 12px;
            font-family: %4;
            font-size: 10px;
            font-weight: bold;
        }
        QPushButton:hover {
            background-color: %5;
            border-color: %6;
        }
        QPushButton:pressed {
            background-color: %7;
        }
        QPushButton:disabled {
            background-color: %8;
            color: %9;
            border-color: %10;
        }
    )").arg(baseColor.name())
       .arg(m_colors.textPrimary.name())
       .arg(m_colors.border.name())
       .arg(m_fonts.bodyFont.family())
       .arg(hoverColor.name())
       .arg(baseColor.name())
       .arg(pressedColor.name())
       .arg(m_colors.backgroundMedium.name())
       .arg(m_colors.textDisabled.name())
       .arg(m_colors.textDisabled.name());
}

QString EnhancedBroadcastTheme::createPanelStyle() const
{
    return QString(R"(
        QWidget {
            background-color: %1;
            color: %2;
            font-family: %3;
            font-size: 10px;
        }
        QFrame {
            background-color: %4;
            border: 1px solid %5;
            border-radius: 4px;
        }
        QGroupBox {
            background-color: %6;
            border: 2px solid %7;
            border-radius: 6px;
            font-weight: bold;
            color: %8;
            margin: 10px 0px;
            padding-top: 15px;
        }
        QGroupBox::title {
            subcontrol-origin: margin;
            left: 10px;
            padding: 0 5px 0 5px;
        }
        QLabel {
            color: %9;
            background-color: transparent;
        }
    )").arg(m_colors.backgroundDark.name())
       .arg(m_colors.textPrimary.name())
       .arg(m_fonts.bodyFont.family())
       .arg(m_colors.backgroundMedium.name())
       .arg(m_colors.border.name())
       .arg(m_colors.backgroundMedium.name())
       .arg(m_colors.border.name())
       .arg(m_colors.textPrimary.name())
       .arg(m_colors.textSecondary.name());
}

QString EnhancedBroadcastTheme::createProgressBarStyle(const QColor& fillColor) const
{
    return QString(R"(
        QProgressBar {
            background-color: %1;
            border: 1px solid %2;
            border-radius: 4px;
            text-align: center;
            color: %3;
            font-weight: bold;
        }
        QProgressBar::chunk {
            background-color: %4;
            border-radius: 3px;
        }
    )").arg(m_colors.backgroundMedium.name())
       .arg(m_colors.border.name())
       .arg(m_colors.textPrimary.name())
       .arg(fillColor.name());
}

QString EnhancedBroadcastTheme::createTableStyle() const
{
    return QString(R"(
        QTableWidget, QTableView {
            background-color: %1;
            color: %2;
            gridline-color: %3;
            border: 1px solid %4;
            selection-background-color: %5;
            selection-color: %6;
            alternate-background-color: %7;
        }
        QTableWidget::item, QTableView::item {
            padding: 4px;
            border: none;
        }
        QTableWidget::item:selected, QTableView::item:selected {
            background-color: %8;
            color: %9;
        }
        QHeaderView::section {
            background-color: %10;
            color: %11;
            border: 1px solid %12;
            padding: 6px;
            font-weight: bold;
        }
        QHeaderView::section:hover {
            background-color: %13;
        }
    )").arg(m_colors.backgroundDark.name())
       .arg(m_colors.textPrimary.name())
       .arg(m_colors.gridMinor.name())
       .arg(m_colors.border.name())
       .arg(m_colors.accentBlue.name())
       .arg(m_colors.textPrimary.name())
       .arg(m_colors.backgroundMedium.name())
       .arg(m_colors.accentBlue.name())
       .arg(m_colors.textPrimary.name())
       .arg(m_colors.backgroundMedium.name())
       .arg(m_colors.textPrimary.name())
       .arg(m_colors.border.name())
       .arg(m_colors.backgroundLight.name());
}

QString EnhancedBroadcastTheme::createTreeViewStyle() const
{
    return QString(R"(
        QTreeWidget, QTreeView {
            background-color: %1;
            color: %2;
            border: 1px solid %3;
            selection-background-color: %4;
            selection-color: %5;
            outline: none;
        }
        QTreeWidget::item, QTreeView::item {
            padding: 3px;
            border: none;
        }
        QTreeWidget::item:selected, QTreeView::item:selected {
            background-color: %6;
            color: %7;
        }
        QTreeWidget::item:hover, QTreeView::item:hover {
            background-color: %8;
        }
        QTreeWidget::branch, QTreeView::branch {
            background-color: transparent;
        }
        QTreeWidget::branch:has-siblings:!adjoins-item, 
        QTreeView::branch:has-siblings:!adjoins-item {
            border-image: none;
            border-left: 1px solid %9;
        }
        QTreeWidget::branch:has-siblings:adjoins-item,
        QTreeView::branch:has-siblings:adjoins-item {
            border-image: none;
            border-left: 1px solid %10;
            border-top: 1px solid %11;
        }
        QTreeWidget::branch:!has-children:!has-siblings:adjoins-item,
        QTreeView::branch:!has-children:!has-siblings:adjoins-item {
            border-image: none;
            border-left: 1px solid %12;
            border-top: 1px solid %13;
        }
    )").arg(m_colors.backgroundDark.name())
       .arg(m_colors.textPrimary.name())
       .arg(m_colors.border.name())
       .arg(m_colors.accentBlue.name())
       .arg(m_colors.textPrimary.name())
       .arg(m_colors.accentBlue.name())
       .arg(m_colors.textPrimary.name())
       .arg(m_colors.backgroundLight.name())
       .arg(m_colors.gridMinor.name())
       .arg(m_colors.gridMinor.name())
       .arg(m_colors.gridMinor.name())
       .arg(m_colors.gridMinor.name())
       .arg(m_colors.gridMinor.name());
}

QString EnhancedBroadcastTheme::createChartStyle() const
{
    return QString(R"(
        QWidget {
            background-color: %1;
            color: %2;
            border: 1px solid %3;
        }
    )").arg(m_colors.chartBackground.name())
       .arg(m_colors.chartForeground.name())
       .arg(m_colors.border.name());
}

QString EnhancedBroadcastTheme::createDockWidgetStyle() const
{
    return QString(R"(
        QDockWidget {
            background-color: %1;
            color: %2;
            titlebar-close-icon: none;
            titlebar-normal-icon: none;
        }
        QDockWidget::title {
            background-color: %3;
            color: %4;
            padding: 6px;
            font-weight: bold;
            text-align: left;
        }
        QDockWidget::close-button, QDockWidget::float-button {
            background-color: %5;
            border: 1px solid %6;
            border-radius: 3px;
            width: 14px;
            height: 14px;
        }
        QDockWidget::close-button:hover, QDockWidget::float-button:hover {
            background-color: %7;
        }
    )").arg(m_colors.backgroundDark.name())
       .arg(m_colors.textPrimary.name())
       .arg(m_colors.backgroundMedium.name())
       .arg(m_colors.textPrimary.name())
       .arg(m_colors.backgroundLight.name())
       .arg(m_colors.border.name())
       .arg(m_colors.accentBlue.name());
}

QString EnhancedBroadcastTheme::createTabWidgetStyle() const
{
    return QString(R"(
        QTabWidget::pane {
            background-color: %1;
            border: 1px solid %2;
            border-radius: 4px;
        }
        QTabWidget::tab-bar {
            alignment: left;
        }
        QTabBar::tab {
            background-color: %3;
            color: %4;
            border: 1px solid %5;
            border-bottom: none;
            border-top-left-radius: 4px;
            border-top-right-radius: 4px;
            padding: 6px 12px;
            margin-right: 2px;
        }
        QTabBar::tab:selected {
            background-color: %6;
            color: %7;
            border-color: %8;
        }
        QTabBar::tab:hover {
            background-color: %9;
        }
        QTabBar::tab:!selected {
            margin-top: 2px;
        }
    )").arg(m_colors.backgroundDark.name())
       .arg(m_colors.border.name())
       .arg(m_colors.backgroundMedium.name())
       .arg(m_colors.textSecondary.name())
       .arg(m_colors.border.name())
       .arg(m_colors.backgroundDark.name())
       .arg(m_colors.textPrimary.name())
       .arg(m_colors.accentBlue.name())
       .arg(m_colors.backgroundLight.name());
}

QString EnhancedBroadcastTheme::createScrollBarStyle() const
{
    return QString(R"(
        QScrollBar:vertical {
            background-color: %1;
            width: 12px;
            border-radius: 6px;
        }
        QScrollBar::handle:vertical {
            background-color: %2;
            border-radius: 6px;
            min-height: 20px;
        }
        QScrollBar::handle:vertical:hover {
            background-color: %3;
        }
        QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical {
            background: none;
            border: none;
        }
        QScrollBar:horizontal {
            background-color: %4;
            height: 12px;
            border-radius: 6px;
        }
        QScrollBar::handle:horizontal {
            background-color: %5;
            border-radius: 6px;
            min-width: 20px;
        }
        QScrollBar::handle:horizontal:hover {
            background-color: %6;
        }
        QScrollBar::add-line:horizontal, QScrollBar::sub-line:horizontal {
            background: none;
            border: none;
        }
    )").arg(m_colors.backgroundMedium.name())
       .arg(m_colors.border.name())
       .arg(m_colors.accentBlue.name())
       .arg(m_colors.backgroundMedium.name())
       .arg(m_colors.border.name())
       .arg(m_colors.accentBlue.name());
}

QString EnhancedBroadcastTheme::createTooltipStyle() const
{
    return QString(R"(
        QToolTip {
            background-color: %1;
            color: %2;
            border: 1px solid %3;
            border-radius: 4px;
            padding: 4px;
            font-size: 9px;
        }
    )").arg(m_colors.backgroundLight.name())
       .arg(m_colors.textPrimary.name())
       .arg(m_colors.border.name());
}

QPixmap EnhancedBroadcastTheme::createServiceIcon(ServiceType type, int size) const
{
    QPixmap pixmap(size, size);
    pixmap.fill(Qt::transparent);
    
    QPainter painter(&pixmap);
    painter.setRenderHint(QPainter::Antialiasing);
    
    QColor color = getServiceTypeColor(type);
    painter.setBrush(QBrush(color));
    painter.setPen(QPen(color.darker(120), 1));
    
    // Draw service type specific shape
    switch (type) {
        case ServiceType::Audio:
            // Speaker icon
            painter.drawEllipse(2, 2, size-4, size-4);
            painter.drawRect(size/2-1, size/4, 2, size/2);
            break;
        case ServiceType::Data:
            // Data squares
            painter.drawRect(2, 2, size/3, size/3);
            painter.drawRect(size/2, 2, size/3, size/3);
            painter.drawRect(2, size/2, size/3, size/3);
            painter.drawRect(size/2, size/2, size/3, size/3);
            break;
        case ServiceType::Packet:
            // Network diamond
            painter.drawPolygon(QPolygon() << QPoint(size/2, 2) 
                                         << QPoint(size-2, size/2)
                                         << QPoint(size/2, size-2)
                                         << QPoint(2, size/2));
            break;
        case ServiceType::Ensemble:
            // Group rectangle
            painter.drawRoundedRect(2, 2, size-4, size-4, 3, 3);
            break;
        default:
            // Unknown - question mark
            painter.drawEllipse(2, 2, size-4, size-4);
            break;
    }
    
    return pixmap;
}

QPixmap EnhancedBroadcastTheme::createQualityIcon(QualityLevel level, int size) const
{
    QPixmap pixmap(size, size);
    pixmap.fill(Qt::transparent);
    
    QPainter painter(&pixmap);
    painter.setRenderHint(QPainter::Antialiasing);
    
    QColor color = getQualityLevelColor(level);
    painter.setBrush(QBrush(color));
    painter.setPen(QPen(color.darker(120), 1));
    
    // Draw quality level bars
    int bars = static_cast<int>(level) + 1;
    int barWidth = size / 5;
    int barSpacing = 1;
    
    for (int i = 0; i < 5; ++i) {
        int barHeight = (size * (i + 1)) / 5;
        int x = i * (barWidth + barSpacing);
        int y = size - barHeight;
        
        if (i < bars) {
            painter.fillRect(x, y, barWidth, barHeight, color);
        } else {
            painter.fillRect(x, y, barWidth, barHeight, m_colors.backgroundMedium);
        }
    }
    
    return pixmap;
}

QPixmap EnhancedBroadcastTheme::createAlertIcon(AlertLevel level, int size) const
{
    QPixmap pixmap(size, size);
    pixmap.fill(Qt::transparent);
    
    QPainter painter(&pixmap);
    painter.setRenderHint(QPainter::Antialiasing);
    
    QColor color = getAlertLevelColor(level);
    painter.setBrush(QBrush(color));
    painter.setPen(QPen(color.darker(120), 1));
    
    // Draw alert level specific shape
    switch (level) {
        case AlertLevel::Info:
            // Info circle with 'i'
            painter.drawEllipse(2, 2, size-4, size-4);
            painter.setPen(QPen(m_colors.textPrimary, 2));
            painter.drawLine(size/2, size/3, size/2, 2*size/3);
            painter.drawPoint(size/2, size/4);
            break;
        case AlertLevel::Warning:
            // Warning triangle
            painter.drawPolygon(QPolygon() << QPoint(size/2, 2)
                                         << QPoint(size-2, size-2)
                                         << QPoint(2, size-2));
            painter.setPen(QPen(m_colors.textPrimary, 2));
            painter.drawLine(size/2, size/3, size/2, size/2);
            painter.drawPoint(size/2, 2*size/3);
            break;
        case AlertLevel::Error:
        case AlertLevel::Critical:
            // Error X
            painter.drawEllipse(2, 2, size-4, size-4);
            painter.setPen(QPen(m_colors.textPrimary, 2));
            painter.drawLine(size/3, size/3, 2*size/3, 2*size/3);
            painter.drawLine(2*size/3, size/3, size/3, 2*size/3);
            break;
    }
    
    return pixmap;
}

QString EnhancedBroadcastTheme::getApplicationStyleSheet() const
{
    return QString(R"(
        QApplication {
            background-color: %1;
            color: %2;
            font-family: %3;
        }
        QMainWindow {
            background-color: %4;
            color: %5;
        }
        QMenuBar {
            background-color: %6;
            color: %7;
            border-bottom: 1px solid %8;
        }
        QMenuBar::item {
            background-color: transparent;
            padding: 4px 8px;
        }
        QMenuBar::item:selected {
            background-color: %9;
        }
        QMenu {
            background-color: %10;
            color: %11;
            border: 1px solid %12;
        }
        QMenu::item {
            padding: 4px 20px;
        }
        QMenu::item:selected {
            background-color: %13;
        }
        QStatusBar {
            background-color: %14;
            color: %15;
            border-top: 1px solid %16;
        }
        QToolBar {
            background-color: %17;
            border: 1px solid %18;
            spacing: 2px;
        }
        QTextEdit {
            background-color: %19;
            color: %20;
            border: 1px solid %21;
            selection-background-color: %22;
        }
        QLineEdit {
            background-color: %23;
            color: %24;
            border: 1px solid %25;
            border-radius: 3px;
            padding: 4px;
        }
        QLineEdit:focus {
            border-color: %26;
        }
        QComboBox {
            background-color: %27;
            color: %28;
            border: 1px solid %29;
            border-radius: 3px;
            padding: 4px;
        }
        QComboBox::drop-down {
            border: none;
        }
        QComboBox::down-arrow {
            width: 12px;
            height: 12px;
        }
        QSpinBox, QDoubleSpinBox {
            background-color: %30;
            color: %31;
            border: 1px solid %32;
            border-radius: 3px;
            padding: 4px;
        }
        QCheckBox {
            color: %33;
            spacing: 8px;
        }
        QCheckBox::indicator {
            width: 16px;
            height: 16px;
            border: 1px solid %34;
            border-radius: 3px;
            background-color: %35;
        }
        QCheckBox::indicator:checked {
            background-color: %36;
        }
        QRadioButton {
            color: %37;
            spacing: 8px;
        }
        QRadioButton::indicator {
            width: 16px;
            height: 16px;
            border: 1px solid %38;
            border-radius: 8px;
            background-color: %39;
        }
        QRadioButton::indicator:checked {
            background-color: %40;
        }
        QSlider::groove:horizontal {
            height: 6px;
            background-color: %41;
            border-radius: 3px;
        }
        QSlider::handle:horizontal {
            background-color: %42;
            border: 1px solid %43;
            width: 16px;
            border-radius: 8px;
            margin: -5px 0;
        }
        QSlider::handle:horizontal:hover {
            background-color: %44;
        }
        %45
        %46
        %47
    )").arg(m_colors.backgroundDark.name())          // 1
       .arg(m_colors.textPrimary.name())             // 2
       .arg(m_fonts.bodyFont.family())               // 3
       .arg(m_colors.backgroundDark.name())          // 4
       .arg(m_colors.textPrimary.name())             // 5
       .arg(m_colors.backgroundMedium.name())        // 6
       .arg(m_colors.textPrimary.name())             // 7
       .arg(m_colors.border.name())                  // 8
       .arg(m_colors.backgroundLight.name())         // 9
       .arg(m_colors.backgroundMedium.name())        // 10
       .arg(m_colors.textPrimary.name())             // 11
       .arg(m_colors.border.name())                  // 12
       .arg(m_colors.accentBlue.name())              // 13
       .arg(m_colors.backgroundMedium.name())        // 14
       .arg(m_colors.textSecondary.name())           // 15
       .arg(m_colors.border.name())                  // 16
       .arg(m_colors.backgroundMedium.name())        // 17
       .arg(m_colors.border.name())                  // 18
       .arg(m_colors.backgroundDark.name())          // 19
       .arg(m_colors.textPrimary.name())             // 20
       .arg(m_colors.border.name())                  // 21
       .arg(m_colors.accentBlue.name())              // 22
       .arg(m_colors.backgroundMedium.name())        // 23
       .arg(m_colors.textPrimary.name())             // 24
       .arg(m_colors.border.name())                  // 25
       .arg(m_colors.accentBlue.name())              // 26
       .arg(m_colors.backgroundMedium.name())        // 27
       .arg(m_colors.textPrimary.name())             // 28
       .arg(m_colors.border.name())                  // 29
       .arg(m_colors.backgroundMedium.name())        // 30
       .arg(m_colors.textPrimary.name())             // 31
       .arg(m_colors.border.name())                  // 32
       .arg(m_colors.textPrimary.name())             // 33
       .arg(m_colors.border.name())                  // 34
       .arg(m_colors.backgroundMedium.name())        // 35
       .arg(m_colors.accentBlue.name())              // 36
       .arg(m_colors.textPrimary.name())             // 37
       .arg(m_colors.border.name())                  // 38
       .arg(m_colors.backgroundMedium.name())        // 39
       .arg(m_colors.accentBlue.name())              // 40
       .arg(m_colors.backgroundMedium.name())        // 41
       .arg(m_colors.accentBlue.name())              // 42
       .arg(m_colors.border.name())                  // 43
       .arg(m_colors.accentBlue.lighter(120).name()) // 44
       .arg(createScrollBarStyle())                  // 45
       .arg(createTooltipStyle())                    // 46
       .arg(createButtonStyle(m_colors.accentBlue)); // 47
}

void EnhancedBroadcastTheme::refreshThemedWidgets()
{
    for (QWidget* widget : m_themedWidgets) {
        if (widget) {
            applyToWidget(widget);
        }
    }
    
    emit themeChanged();
}

QPalette EnhancedBroadcastTheme::createBroadcastPalette() const
{
    QPalette palette;
    
    // Window colors
    palette.setColor(QPalette::Window, m_colors.backgroundDark);
    palette.setColor(QPalette::WindowText, m_colors.textPrimary);
    
    // Base colors (input fields)
    palette.setColor(QPalette::Base, m_colors.backgroundMedium);
    palette.setColor(QPalette::Text, m_colors.textPrimary);
    
    // Button colors
    palette.setColor(QPalette::Button, m_colors.backgroundMedium);
    palette.setColor(QPalette::ButtonText, m_colors.textPrimary);
    
    // Highlight colors
    palette.setColor(QPalette::Highlight, m_colors.accentBlue);
    palette.setColor(QPalette::HighlightedText, m_colors.textPrimary);
    
    // Disabled colors
    palette.setColor(QPalette::Disabled, QPalette::WindowText, m_colors.textDisabled);
    palette.setColor(QPalette::Disabled, QPalette::Text, m_colors.textDisabled);
    palette.setColor(QPalette::Disabled, QPalette::ButtonText, m_colors.textDisabled);
    
    return palette;
}

void EnhancedBroadcastTheme::createWidgetStyles()
{
    // Pre-cache common widget styles
    m_styleCache["button_primary"] = createButtonStyle(m_colors.accentBlue);
    m_styleCache["button_success"] = createButtonStyle(m_colors.accentGreen);
    m_styleCache["button_warning"] = createButtonStyle(m_colors.accentYellow);
    m_styleCache["button_danger"] = createButtonStyle(m_colors.accentRed);
    
    m_styleCache["progress_excellent"] = createProgressBarStyle(m_colors.qualityExcellent);
    m_styleCache["progress_good"] = createProgressBarStyle(m_colors.qualityGood);
    m_styleCache["progress_fair"] = createProgressBarStyle(m_colors.qualityFair);
    m_styleCache["progress_poor"] = createProgressBarStyle(m_colors.qualityPoor);
    m_styleCache["progress_critical"] = createProgressBarStyle(m_colors.qualityCritical);
}

void EnhancedBroadcastTheme::initializeColorMappings()
{
    // Color mappings are handled by the enum functions
    // This method is available for future extensions
}