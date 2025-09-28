#pragma once

#include <QObject>
#include <QColor>
#include <QBrush>
#include <QPen>
#include <QFont>
#include <QGradient>
#include <QPalette>
#include <QWidget>
#include <QStyle>
#include <QStyleOption>
#include <QApplication>
#include <QString>
#include <QHash>

/**
 * @enum ServiceType
 * @brief Professional service type enumeration for color coding
 */
enum class ServiceType {
    Audio,          ///< DAB audio services (green)
    Data,           ///< Data services (orange)
    Packet,         ///< Packet mode services (blue)
    Ensemble,       ///< Ensemble information (purple)
    Unknown         ///< Unknown service type (gray)
};

/**
 * @enum QualityLevel
 * @brief Signal quality levels for professional monitoring
 */
enum class QualityLevel {
    Excellent,      ///< >95% quality (bright green)
    Good,           ///< 80-95% quality (green)
    Fair,           ///< 60-80% quality (yellow)
    Poor,           ///< 40-60% quality (orange)
    Critical        ///< <40% quality (red)
};

/**
 * @enum AlertLevel
 * @brief Professional alert levels for broadcast monitoring
 */
enum class AlertLevel {
    Info,           ///< Informational (blue)
    Warning,        ///< Warning (amber)
    Error,          ///< Error (red)
    Critical        ///< Critical error (dark red)
};

/**
 * @struct BroadcastColors
 * @brief Professional broadcast industry color palette
 */
struct BroadcastColors {
    // Background colors
    QColor backgroundDark;      // #2D2D30 - Main background
    QColor backgroundMedium;    // #3E3E42 - Panel backgrounds
    QColor backgroundLight;     // #4A4A4F - Hover states
    
    // Accent colors
    QColor accentBlue;          // #0078D4 - Primary accent
    QColor accentOrange;        // #FF8C00 - Secondary accent
    QColor accentGreen;         // #00AA00 - Success/Good
    QColor accentRed;           // #DC3545 - Error/Critical
    QColor accentYellow;        // #FFC107 - Warning/Fair
    
    // Text colors
    QColor textPrimary;         // #FFFFFF - Primary text
    QColor textSecondary;       // #CCCCCC - Secondary text
    QColor textMuted;           // #999999 - Muted text
    QColor textDisabled;        // #666666 - Disabled text
    
    // Service type colors
    QColor serviceAudio;        // #00C851 - Audio services
    QColor serviceData;         // #FF8800 - Data services
    QColor servicePacket;       // #007BFF - Packet services
    QColor serviceEnsemble;     // #6F42C1 - Ensemble info
    QColor serviceUnknown;      // #868E96 - Unknown services
    
    // Quality level colors
    QColor qualityExcellent;    // #28A745 - Excellent quality
    QColor qualityGood;         // #6BBF59 - Good quality
    QColor qualityFair;         // #FFC107 - Fair quality
    QColor qualityPoor;         // #FD7E14 - Poor quality
    QColor qualityCritical;     // #DC3545 - Critical quality
    
    // Alert level colors
    QColor alertInfo;           // #17A2B8 - Info alerts
    QColor alertWarning;        // #FFC107 - Warning alerts
    QColor alertError;          // #DC3545 - Error alerts
    QColor alertCritical;       // #A41E22 - Critical alerts
    
    // Grid and axis colors
    QColor gridMajor;           // #5A5A5A - Major grid lines
    QColor gridMinor;           // #404040 - Minor grid lines
    QColor axis;                // #808080 - Axis lines
    QColor border;              // #6C757D - Borders
    
    // Chart specific colors
    QColor chartBackground;     // #252528 - Chart background
    QColor chartForeground;     // #FFFFFF - Chart foreground
    QColor targetLine;          // #FFC107 - Target threshold lines
    QColor trendLine;           // #00D4AA - Trend lines
    
    BroadcastColors() {
        // Initialize professional broadcast colors
        backgroundDark = QColor(0x2D, 0x2D, 0x30);
        backgroundMedium = QColor(0x3E, 0x3E, 0x42);
        backgroundLight = QColor(0x4A, 0x4A, 0x4F);
        
        accentBlue = QColor(0x00, 0x78, 0xD4);
        accentOrange = QColor(0xFF, 0x8C, 0x00);
        accentGreen = QColor(0x00, 0xAA, 0x00);
        accentRed = QColor(0xDC, 0x35, 0x45);
        accentYellow = QColor(0xFF, 0xC1, 0x07);
        
        textPrimary = QColor(0xFF, 0xFF, 0xFF);
        textSecondary = QColor(0xCC, 0xCC, 0xCC);
        textMuted = QColor(0x99, 0x99, 0x99);
        textDisabled = QColor(0x66, 0x66, 0x66);
        
        serviceAudio = QColor(0x00, 0xC8, 0x51);
        serviceData = QColor(0xFF, 0x88, 0x00);
        servicePacket = QColor(0x00, 0x7B, 0xFF);
        serviceEnsemble = QColor(0x6F, 0x42, 0xC1);
        serviceUnknown = QColor(0x86, 0x8E, 0x96);
        
        qualityExcellent = QColor(0x28, 0xA7, 0x45);
        qualityGood = QColor(0x6B, 0xBF, 0x59);
        qualityFair = QColor(0xFF, 0xC1, 0x07);
        qualityPoor = QColor(0xFD, 0x7E, 0x14);
        qualityCritical = QColor(0xDC, 0x35, 0x45);
        
        alertInfo = QColor(0x17, 0xA2, 0xB8);
        alertWarning = QColor(0xFF, 0xC1, 0x07);
        alertError = QColor(0xDC, 0x35, 0x45);
        alertCritical = QColor(0xA4, 0x1E, 0x22);
        
        gridMajor = QColor(0x5A, 0x5A, 0x5A);
        gridMinor = QColor(0x40, 0x40, 0x40);
        axis = QColor(0x80, 0x80, 0x80);
        border = QColor(0x6C, 0x75, 0x7D);
        
        chartBackground = QColor(0x25, 0x25, 0x28);
        chartForeground = QColor(0xFF, 0xFF, 0xFF);
        targetLine = QColor(0xFF, 0xC1, 0x07);
        trendLine = QColor(0x00, 0xD4, 0xAA);
    }
};

/**
 * @struct BroadcastFonts
 * @brief Professional typography system for broadcast interfaces
 */
struct BroadcastFonts {
    QFont titleFont;            // Large titles and headers
    QFont headerFont;           // Section headers
    QFont bodyFont;             // Regular body text
    QFont captionFont;          // Small captions and labels
    QFont monospaceFont;        // Technical data display
    QFont statusFont;           // Status indicators
    
    BroadcastFonts() {
        titleFont = QFont("Segoe UI", 14, QFont::Bold);
        headerFont = QFont("Segoe UI", 12, QFont::DemiBold);
        bodyFont = QFont("Segoe UI", 10);
        captionFont = QFont("Segoe UI", 9);
        monospaceFont = QFont("Consolas", 9);
        statusFont = QFont("Segoe UI", 8, QFont::Bold);
    }
};

/**
 * @class EnhancedBroadcastTheme
 * @brief Professional broadcast industry theme manager
 * 
 * This class provides comprehensive theming for professional broadcast
 * monitoring applications with industry-standard color schemes,
 * typography, and visual indicators.
 */
class EnhancedBroadcastTheme : public QObject
{
    Q_OBJECT

public:
    /**
     * @brief Get singleton instance
     * @return Theme manager instance
     */
    static EnhancedBroadcastTheme& instance();

    /**
     * @brief Initialize theme system
     */
    void initialize();

    /**
     * @brief Apply theme to application
     * @param app Application instance
     */
    void applyToApplication(QApplication* app);

    /**
     * @brief Apply theme to widget
     * @param widget Target widget
     */
    void applyToWidget(QWidget* widget);

    /**
     * @brief Get broadcast colors
     * @return Color palette
     */
    const BroadcastColors& colors() const { return m_colors; }

    /**
     * @brief Get broadcast fonts
     * @return Font system
     */
    const BroadcastFonts& fonts() const { return m_fonts; }

    /**
     * @brief Get service type color
     * @param type Service type
     * @return Service color
     */
    QColor getServiceTypeColor(ServiceType type) const;

    /**
     * @brief Get quality level color
     * @param level Quality level
     * @return Quality color
     */
    QColor getQualityLevelColor(QualityLevel level) const;

    /**
     * @brief Get alert level color
     * @param level Alert level
     * @return Alert color
     */
    QColor getAlertLevelColor(AlertLevel level) const;

    /**
     * @brief Create professional gradient
     * @param startColor Start color
     * @param endColor End color
     * @param orientation Gradient orientation
     * @return Gradient brush
     */
    QLinearGradient createGradient(const QColor& startColor, 
                                   const QColor& endColor,
                                   Qt::Orientation orientation = Qt::Vertical) const;

    /**
     * @brief Create professional button style
     * @param baseColor Base button color
     * @return Button stylesheet
     */
    QString createButtonStyle(const QColor& baseColor) const;

    /**
     * @brief Create professional panel style
     * @return Panel stylesheet
     */
    QString createPanelStyle() const;

    /**
     * @brief Create professional progress bar style
     * @param fillColor Progress fill color
     * @return Progress bar stylesheet
     */
    QString createProgressBarStyle(const QColor& fillColor) const;

    /**
     * @brief Create professional table style
     * @return Table stylesheet
     */
    QString createTableStyle() const;

    /**
     * @brief Create professional tree view style
     * @return Tree view stylesheet
     */
    QString createTreeViewStyle() const;

    /**
     * @brief Create professional chart style
     * @return Chart widget stylesheet
     */
    QString createChartStyle() const;

    /**
     * @brief Create professional dock widget style
     * @return Dock widget stylesheet
     */
    QString createDockWidgetStyle() const;

    /**
     * @brief Create professional tab widget style
     * @return Tab widget stylesheet
     */
    QString createTabWidgetStyle() const;

    /**
     * @brief Create professional scroll bar style
     * @return Scroll bar stylesheet
     */
    QString createScrollBarStyle() const;

    /**
     * @brief Create professional tooltip style
     * @return Tooltip stylesheet
     */
    QString createTooltipStyle() const;

    /**
     * @brief Create service indicator icon
     * @param type Service type
     * @param size Icon size
     * @return Service icon pixmap
     */
    QPixmap createServiceIcon(ServiceType type, int size = 16) const;

    /**
     * @brief Create quality indicator icon
     * @param level Quality level
     * @param size Icon size
     * @return Quality icon pixmap
     */
    QPixmap createQualityIcon(QualityLevel level, int size = 16) const;

    /**
     * @brief Create alert indicator icon
     * @param level Alert level
     * @param size Icon size
     * @return Alert icon pixmap
     */
    QPixmap createAlertIcon(AlertLevel level, int size = 16) const;

    /**
     * @brief Get professional application stylesheet
     * @return Complete application stylesheet
     */
    QString getApplicationStyleSheet() const;

public slots:
    /**
     * @brief Refresh all themed widgets
     */
    void refreshThemedWidgets();

signals:
    /**
     * @brief Emitted when theme changes
     */
    void themeChanged();

private:
    explicit EnhancedBroadcastTheme(QObject *parent = nullptr);
    ~EnhancedBroadcastTheme() = default;

    // Singleton pattern
    EnhancedBroadcastTheme(const EnhancedBroadcastTheme&) = delete;
    EnhancedBroadcastTheme& operator=(const EnhancedBroadcastTheme&) = delete;

    /**
     * @brief Create complete application palette
     * @return Professional palette
     */
    QPalette createBroadcastPalette() const;

    /**
     * @brief Create professional widget styling
     * @return Widget-specific stylesheets
     */
    void createWidgetStyles();

    /**
     * @brief Initialize color mappings
     */
    void initializeColorMappings();

    // Theme data
    BroadcastColors m_colors;
    BroadcastFonts m_fonts;
    
    // Widget style cache
    QHash<QString, QString> m_styleCache;
    
    // Tracked widgets for theme updates
    QList<QWidget*> m_themedWidgets;
    
    // Initialization state
    bool m_initialized;
};

// Convenience macros for theme access
#define BROADCAST_THEME EnhancedBroadcastTheme::instance()
#define BROADCAST_COLORS BROADCAST_THEME.colors()
#define BROADCAST_FONTS BROADCAST_THEME.fonts()

// Helper functions for quick color access
inline QColor getServiceColor(ServiceType type) {
    return BROADCAST_THEME.getServiceTypeColor(type);
}

inline QColor getQualityColor(QualityLevel level) {
    return BROADCAST_THEME.getQualityLevelColor(level);
}

inline QColor getAlertColor(AlertLevel level) {
    return BROADCAST_THEME.getAlertLevelColor(level);
}