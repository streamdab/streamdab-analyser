#pragma once

#include <QColor>
#include <QString>

/**
 * @class BroadcastTheme
 * @brief Professional broadcast industry color scheme and typography
 * 
 * Implements the professional color palette and typography standards
 * used in broadcast industry tools like Elecard, DekTec, and Rohde & Schwarz.
 * 
 * Color Coding Philosophy:
 * - Green: Audio services, good status, successful operations
 * - Orange: Data services, warnings, moderate issues  
 * - Blue: Ensemble information, selected items, primary actions
 * - Red: Critical errors, failed operations, alerts
 * - Purple: MSC data, special protocols, advanced features
 * - Gray: Disabled items, background information, secondary text
 */
class BroadcastTheme
{
public:
    // ═══════════════════════════════════════════════════════════════════
    // PROFESSIONAL BROADCAST COLOR PALETTE
    // ═══════════════════════════════════════════════════════════════════
    
    // Primary Theme Colors
    static const QColor BACKGROUND_DARK;        // #2D2D30 - Main background
    static const QColor BACKGROUND_MEDIUM;      // #3E3E42 - Panel backgrounds
    static const QColor BACKGROUND_LIGHT;       // #F8F8F8 - Content areas
    static const QColor ACCENT_BLUE;            // #0078D4 - Primary accent
    static const QColor ACCENT_BLUE_DARK;       // #005A9E - Dark accent
    static const QColor ACCENT_BLUE_LIGHT;      // #CCE8FF - Light accent
    
    // Service Type Color Coding (Industry Standard)
    static const QColor SERVICE_AUDIO;          // #00A000 - Audio services (Green)
    static const QColor SERVICE_DATA;           // #FF8C00 - Data services (Orange)
    static const QColor SERVICE_ENSEMBLE;       // #0078D4 - Ensemble info (Blue)
    static const QColor SERVICE_PACKET;         // #9C27B0 - Packet mode (Purple)
    static const QColor SERVICE_UNKNOWN;        // #808080 - Unknown type (Gray)
    
    // Status Indication Colors (Broadcast Standard)
    static const QColor STATUS_OK;              // #4CAF50 - Good/OK status
    static const QColor STATUS_WARNING;         // #FF9800 - Warning status
    static const QColor STATUS_ERROR;           // #F44336 - Error status
    static const QColor STATUS_CRITICAL;        // #DC3545 - Critical error
    static const QColor STATUS_INFO;            // #2196F3 - Information
    
    // ETI Frame Type Colors
    static const QColor FRAME_FIC;              // #2196F3 - FIC data (Blue)
    static const QColor FRAME_MSC;              // #9C27B0 - MSC data (Purple)
    static const QColor FRAME_SYNC;             // #4CAF50 - Sync frames (Green)
    static const QColor FRAME_ERROR;            // #F44336 - Error frames (Red)
    static const QColor FRAME_PADDING;          // #9E9E9E - Padding (Gray)
    
    // Quality Indication Colors
    static const QColor QUALITY_EXCELLENT;      // #4CAF50 - >95% quality
    static const QColor QUALITY_GOOD;           // #8BC34A - 85-95% quality  
    static const QColor QUALITY_FAIR;           // #FFC107 - 70-85% quality
    static const QColor QUALITY_POOR;           // #FF9800 - 50-70% quality
    static const QColor QUALITY_BAD;            // #F44336 - <50% quality
    
    // Typography Definitions
    static const QString FONT_FAMILY_UI;        // "Segoe UI" - UI elements
    static const QString FONT_FAMILY_MONO;      // "Consolas" - Technical data
    static const QString FONT_FAMILY_HEADER;    // "Segoe UI" - Headers
    
    // Font Sizes (Professional Broadcasting Standard)
    static const int FONT_SIZE_HEADER;          // 12px - Panel headers
    static const int FONT_SIZE_UI;              // 10px - UI elements
    static const int FONT_SIZE_TECHNICAL;       // 9px - Technical data
    static const int FONT_SIZE_SMALL;           // 8px - Small details
    
    // ═══════════════════════════════════════════════════════════════════
    // THEME APPLICATION METHODS
    // ═══════════════════════════════════════════════════════════════════
    
    /**
     * @brief Get comprehensive stylesheet for entire application
     * @return Complete CSS stylesheet string
     */
    static QString getMainApplicationStyleSheet();
    
    /**
     * @brief Get stylesheet for service tree (color-coded by service type)
     * @return Service tree specific stylesheet
     */
    static QString getServiceTreeStyleSheet();
    
    /**
     * @brief Get stylesheet for frame listing table (color-coded by frame type)
     * @return Frame table specific stylesheet
     */
    static QString getFrameTableStyleSheet();
    
    /**
     * @brief Get stylesheet for properties panel (professional parameter display)
     * @return Properties panel specific stylesheet
     */
    static QString getPropertiesPanelStyleSheet();
    
    /**
     * @brief Get stylesheet for bottom tool panels (hex viewer, logs, etc.)
     * @return Bottom panels specific stylesheet
     */
    static QString getBottomPanelsStyleSheet();
    
    // ═══════════════════════════════════════════════════════════════════
    // SERVICE TYPE COLOR MAPPING
    // ═══════════════════════════════════════════════════════════════════
    
    /**
     * @brief Get color for DAB service type
     * @param serviceType Service type identifier
     * @return Appropriate color for service type
     */
    static QColor getServiceTypeColor(int serviceType);
    
    /**
     * @brief Get color for service quality level
     * @param qualityPercent Quality percentage (0-100)
     * @return Appropriate color for quality level
     */
    static QColor getQualityColor(double qualityPercent);
    
    /**
     * @brief Get color for ETI frame status
     * @param hasErrors True if frame has errors
     * @param frameType Frame type identifier
     * @return Appropriate color for frame status
     */
    static QColor getFrameStatusColor(bool hasErrors, const QString& frameType);
    
    /**
     * @brief Get color for FIG (Fast Information Group) type
     * @param figType FIG type number (0-31)
     * @return Appropriate color for FIG type
     */
    static QColor getFigTypeColor(int figType);
    
    // ═══════════════════════════════════════════════════════════════════
    // PROFESSIONAL UI HELPER METHODS
    // ═══════════════════════════════════════════════════════════════════
    
    /**
     * @brief Create professional button style with hover effects
     * @param baseColor Base button color
     * @param textColor Text color
     * @return Button stylesheet string
     */
    static QString createButtonStyle(const QColor& baseColor, const QColor& textColor = QColor("#333333"));
    
    /**
     * @brief Create professional panel header style
     * @param title Panel title text
     * @param backgroundColor Header background color
     * @return Header stylesheet string
     */
    static QString createPanelHeaderStyle(const QString& title, const QColor& backgroundColor = ACCENT_BLUE);
    
    /**
     * @brief Create status indicator LED style
     * @param isActive True for active status
     * @param statusColor Color for active state
     * @return LED indicator stylesheet
     */
    static QString createStatusLedStyle(bool isActive, const QColor& statusColor = STATUS_OK);
    
    // ═══════════════════════════════════════════════════════════════════
    // COLOR UTILITY METHODS
    // ═══════════════════════════════════════════════════════════════════
    
    /**
     * @brief Convert color to hex string for CSS
     * @param color QColor to convert
     * @return Hex string representation
     */
    static QString colorToHex(const QColor& color);
    
    /**
     * @brief Create gradient string for CSS
     * @param startColor Gradient start color
     * @param endColor Gradient end color
     * @param direction Gradient direction ("vertical" or "horizontal")
     * @return CSS gradient string
     */
    static QString createGradient(const QColor& startColor, const QColor& endColor, 
                                 const QString& direction = "vertical");
    
    /**
     * @brief Apply transparency to color
     * @param color Base color
     * @param alpha Transparency (0.0-1.0)
     * @return Color with applied transparency
     */
    static QColor withAlpha(const QColor& color, double alpha);
};