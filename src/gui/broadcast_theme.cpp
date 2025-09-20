#include "broadcast_theme.h"
#include <QStringBuilder>

// ═══════════════════════════════════════════════════════════════════
// PROFESSIONAL BROADCAST COLOR PALETTE IMPLEMENTATION
// ═══════════════════════════════════════════════════════════════════

// Primary Theme Colors
const QColor BroadcastTheme::BACKGROUND_DARK(0x2D, 0x2D, 0x30);        // #2D2D30
const QColor BroadcastTheme::BACKGROUND_MEDIUM(0x3E, 0x3E, 0x42);      // #3E3E42
const QColor BroadcastTheme::BACKGROUND_LIGHT(0xF8, 0xF8, 0xF8);       // #F8F8F8
const QColor BroadcastTheme::ACCENT_BLUE(0x00, 0x78, 0xD4);            // #0078D4
const QColor BroadcastTheme::ACCENT_BLUE_DARK(0x00, 0x5A, 0x9E);       // #005A9E
const QColor BroadcastTheme::ACCENT_BLUE_LIGHT(0xCC, 0xE8, 0xFF);      // #CCE8FF

// Service Type Color Coding (Industry Standard)
const QColor BroadcastTheme::SERVICE_AUDIO(0x00, 0xA0, 0x00);          // #00A000 - Green
const QColor BroadcastTheme::SERVICE_DATA(0xFF, 0x8C, 0x00);           // #FF8C00 - Orange
const QColor BroadcastTheme::SERVICE_ENSEMBLE(0x00, 0x78, 0xD4);       // #0078D4 - Blue
const QColor BroadcastTheme::SERVICE_PACKET(0x9C, 0x27, 0xB0);         // #9C27B0 - Purple
const QColor BroadcastTheme::SERVICE_UNKNOWN(0x80, 0x80, 0x80);        // #808080 - Gray

// Status Indication Colors (Broadcast Standard)
const QColor BroadcastTheme::STATUS_OK(0x4C, 0xAF, 0x50);              // #4CAF50 - Green
const QColor BroadcastTheme::STATUS_WARNING(0xFF, 0x98, 0x00);         // #FF9800 - Orange
const QColor BroadcastTheme::STATUS_ERROR(0xF4, 0x43, 0x36);           // #F44336 - Red
const QColor BroadcastTheme::STATUS_CRITICAL(0xDC, 0x35, 0x45);        // #DC3545 - Dark Red
const QColor BroadcastTheme::STATUS_INFO(0x21, 0x96, 0xF3);            // #2196F3 - Blue

// ETI Frame Type Colors
const QColor BroadcastTheme::FRAME_FIC(0x21, 0x96, 0xF3);              // #2196F3 - Blue
const QColor BroadcastTheme::FRAME_MSC(0x9C, 0x27, 0xB0);              // #9C27B0 - Purple
const QColor BroadcastTheme::FRAME_SYNC(0x4C, 0xAF, 0x50);             // #4CAF50 - Green
const QColor BroadcastTheme::FRAME_ERROR(0xF4, 0x43, 0x36);            // #F44336 - Red
const QColor BroadcastTheme::FRAME_PADDING(0x9E, 0x9E, 0x9E);          // #9E9E9E - Gray

// Quality Indication Colors
const QColor BroadcastTheme::QUALITY_EXCELLENT(0x4C, 0xAF, 0x50);      // #4CAF50 - Green
const QColor BroadcastTheme::QUALITY_GOOD(0x8B, 0xC3, 0x4A);           // #8BC34A - Light Green
const QColor BroadcastTheme::QUALITY_FAIR(0xFF, 0xC1, 0x07);           // #FFC107 - Yellow
const QColor BroadcastTheme::QUALITY_POOR(0xFF, 0x98, 0x00);           // #FF9800 - Orange
const QColor BroadcastTheme::QUALITY_BAD(0xF4, 0x43, 0x36);            // #F44336 - Red

// Typography Definitions
const QString BroadcastTheme::FONT_FAMILY_UI = "Segoe UI";
const QString BroadcastTheme::FONT_FAMILY_MONO = "Consolas";
const QString BroadcastTheme::FONT_FAMILY_HEADER = "Segoe UI";

// Font Sizes (Professional Broadcasting Standard)
const int BroadcastTheme::FONT_SIZE_HEADER = 12;
const int BroadcastTheme::FONT_SIZE_UI = 10;
const int BroadcastTheme::FONT_SIZE_TECHNICAL = 9;
const int BroadcastTheme::FONT_SIZE_SMALL = 8;

// ═══════════════════════════════════════════════════════════════════
// COMPREHENSIVE PROFESSIONAL STYLESHEET IMPLEMENTATION
// ═══════════════════════════════════════════════════════════════════

QString BroadcastTheme::getMainApplicationStyleSheet()
{
    return QString(
        "/* ═══════════════════════════════════════════════════════════════════ */\n"
        "/* ETI STREAM ANALYZER - PROFESSIONAL BROADCAST INDUSTRY THEME          */\n"
        "/* Following Elecard/DekTec/R&S industry standards                      */\n"
        "/* ═══════════════════════════════════════════════════════════════════ */\n"
        "\n"
        "QMainWindow {\n"
        "    background-color: %1;\n"
        "    color: #333333;\n"
        "    font-family: '%2';\n"
        "    font-size: %3px;\n"
        "}\n"
        "\n"
        "/* ═══════════════════════════════════════════════════════════════════ */\n"
        "/* PROFESSIONAL PANEL HEADERS                                            */\n"
        "/* ═══════════════════════════════════════════════════════════════════ */\n"
        "\n"
        "QLabel#PanelHeader {\n"
        "    font-family: '%4';\n"
        "    font-weight: bold;\n"
        "    font-size: %5px;\n"
        "    color: #FFFFFF;\n"
        "    padding: 8px 12px;\n"
        "    background: %6;\n"
        "    border: 1px solid %7;\n"
        "    border-radius: 4px;\n"
        "    margin-bottom: 2px;\n"
        "}\n"
        "\n"
        "/* ═══════════════════════════════════════════════════════════════════ */\n"
        "/* PROFESSIONAL SPLITTERS WITH BROADCAST INDUSTRY STYLING               */\n"
        "/* ═══════════════════════════════════════════════════════════════════ */\n"
        "\n"
        "QSplitter::handle {\n"
        "    background: %8;\n"
        "    border: 1px solid #CCCCCC;\n"
        "    border-radius: 3px;\n"
        "}\n"
        "\n"
        "QSplitter::handle:horizontal {\n"
        "    width: 6px;\n"
        "    margin: 2px 0px;\n"
        "}\n"
        "\n"
        "QSplitter::handle:vertical {\n"
        "    height: 6px;\n"
        "    margin: 0px 2px;\n"
        "}\n"
        "\n"
        "QSplitter::handle:hover {\n"
        "    background: %9;\n"
        "}\n"
        "\n"
        "/* ═══════════════════════════════════════════════════════════════════ */\n"
        "/* PROFESSIONAL TAB WIDGETS - BROADCAST INDUSTRY STANDARD               */\n"
        "/* ═══════════════════════════════════════════════════════════════════ */\n"
        "\n"
        "QTabWidget::pane {\n"
        "    border: 1px solid #CCCCCC;\n"
        "    background-color: #FFFFFF;\n"
        "    border-radius: 4px;\n"
        "    margin-top: 1px;\n"
        "}\n"
        "\n"
        "QTabBar::tab {\n"
        "    background: %10;\n"
        "    border: 1px solid #CCCCCC;\n"
        "    padding: 8px 16px;\n"
        "    margin-right: 2px;\n"
        "    border-radius: 4px 4px 0px 0px;\n"
        "    font-family: '%11';\n"
        "    font-size: %12px;\n"
        "    color: #333333;\n"
        "    min-width: 80px;\n"
        "}\n"
        "\n"
        "QTabBar::tab:selected {\n"
        "    background: %13;\n"
        "    border-bottom: 2px solid %14;\n"
        "    color: %15;\n"
        "    font-weight: bold;\n"
        "}\n"
        "\n"
        "QTabBar::tab:hover:!selected {\n"
        "    background: %16;\n"
        "    color: %17;\n"
        "}\n"
        "\n"
        "/* ═══════════════════════════════════════════════════════════════════ */\n"
        "/* PROFESSIONAL BUTTONS & CONTROLS                                       */\n"
        "/* ═══════════════════════════════════════════════════════════════════ */\n"
        "\n"
        "QPushButton {\n"
        "    background: %18;\n"
        "    border: 1px solid #CCCCCC;\n"
        "    border-radius: 4px;\n"
        "    padding: 6px 12px;\n"
        "    font-family: '%19';\n"
        "    font-size: %20px;\n"
        "    font-weight: normal;\n"
        "    color: #333333;\n"
        "}\n"
        "\n"
        "QPushButton:hover {\n"
        "    background: %21;\n"
        "    border-color: %22;\n"
        "    color: %23;\n"
        "}\n"
        "\n"
        "QPushButton:pressed {\n"
        "    background: %24;\n"
        "    border-color: %25;\n"
        "}\n"
        "\n"
        "QPushButton:checked {\n"
        "    background: %26;\n"
        "    color: #FFFFFF;\n"
        "    border-color: %27;\n"
        "    font-weight: bold;\n"
        "}\n"
        "\n"
        "QPushButton:disabled {\n"
        "    background-color: #F0F0F0;\n"
        "    border-color: #D0D0D0;\n"
        "    color: #A0A0A0;\n"
        "}\n"
        "\n"
        "/* ═══════════════════════════════════════════════════════════════════ */\n"
        "/* PROFESSIONAL INPUT FIELDS                                             */\n"
        "/* ═══════════════════════════════════════════════════════════════════ */\n"
        "\n"
        "QLineEdit, QComboBox {\n"
        "    border: 1px solid #CCCCCC;\n"
        "    border-radius: 3px;\n"
        "    padding: 4px 8px;\n"
        "    background-color: #FFFFFF;\n"
        "    font-family: '%28';\n"
        "    font-size: %29px;\n"
        "    selection-background-color: %30;\n"
        "}\n"
        "\n"
        "QLineEdit:focus, QComboBox:focus {\n"
        "    border: 2px solid %31;\n"
        "    outline: none;\n"
        "    background-color: #F8FCFF;\n"
        "}\n"
        "\n"
        "/* ═══════════════════════════════════════════════════════════════════ */\n"
        "/* PROFESSIONAL TEXT EDITORS FOR TECHNICAL DATA                         */\n"
        "/* ═══════════════════════════════════════════════════════════════════ */\n"
        "\n"
        "QTextEdit {\n"
        "    background-color: #FAFAFA;\n"
        "    border: 1px solid #E0E0E0;\n"
        "    border-radius: 3px;\n"
        "    selection-background-color: %32;\n"
        "    font-family: '%33';\n"
        "    font-size: %34px;\n"
        "    line-height: 1.3;\n"
        "    padding: 6px;\n"
        "}\n"
    ).arg(
        colorToHex(BACKGROUND_LIGHT),                                    // %1 - Main background
        FONT_FAMILY_UI,                                                  // %2 - Main font family
        QString::number(FONT_SIZE_UI),                                   // %3 - Main font size
        FONT_FAMILY_HEADER,                                              // %4 - Header font family
        QString::number(FONT_SIZE_HEADER),                               // %5 - Header font size
        createGradient(ACCENT_BLUE, ACCENT_BLUE_DARK),                   // %6 - Header background gradient
        colorToHex(ACCENT_BLUE_DARK),                                    // %7 - Header border color
        createGradient(QColor("#F0F0F0"), QColor("#E0E0E0")),           // %8 - Splitter handle gradient
        createGradient(ACCENT_BLUE_LIGHT, ACCENT_BLUE),                 // %9 - Splitter handle hover
        createGradient(QColor("#F0F0F0"), QColor("#E0E0E0")),           // %10 - Tab background
        FONT_FAMILY_UI,                                                  // %11 - Tab font family
        QString::number(FONT_SIZE_UI),                                   // %12 - Tab font size
        createGradient(QColor("#FFFFFF"), QColor("#F8F8F8")),           // %13 - Selected tab background
        colorToHex(ACCENT_BLUE),                                         // %14 - Selected tab border
        colorToHex(ACCENT_BLUE),                                         // %15 - Selected tab text color
        createGradient(ACCENT_BLUE_LIGHT, QColor("#CCE8FF")),           // %16 - Hover tab background
        colorToHex(ACCENT_BLUE),                                         // %17 - Hover tab text color
        createGradient(QColor("#F8F8F8"), QColor("#E8E8E8")),           // %18 - Button background
        FONT_FAMILY_UI,                                                  // %19 - Button font family
        QString::number(FONT_SIZE_UI),                                   // %20 - Button font size
        createGradient(QColor("#E8F4FF"), QColor("#CCE8FF")),           // %21 - Button hover background
        colorToHex(ACCENT_BLUE),                                         // %22 - Button hover border
        colorToHex(ACCENT_BLUE),                                         // %23 - Button hover text
        createGradient(ACCENT_BLUE_LIGHT, QColor("#B3D9FF")),           // %24 - Button pressed background
        colorToHex(ACCENT_BLUE_DARK),                                    // %25 - Button pressed border
        createGradient(ACCENT_BLUE, ACCENT_BLUE_DARK),                   // %26 - Button checked background
        colorToHex(ACCENT_BLUE_DARK),                                    // %27 - Button checked border
        FONT_FAMILY_UI,                                                  // %28 - Input font family
        QString::number(FONT_SIZE_UI),                                   // %29 - Input font size
        colorToHex(ACCENT_BLUE_LIGHT),                                   // %30 - Selection background
        colorToHex(ACCENT_BLUE),                                         // %31 - Focus border color
        colorToHex(ACCENT_BLUE_LIGHT),                                   // %32 - Text selection background
        FONT_FAMILY_MONO,                                                // %33 - Text editor font family
        QString::number(FONT_SIZE_TECHNICAL)                             // %34 - Text editor font size
    );
}

QString BroadcastTheme::getServiceTreeStyleSheet()
{
    return QString(
        "/* ═══════════════════════════════════════════════════════════════════ */\n"
        "/* SERVICE TREE WITH DAB SERVICE TYPE COLOR CODING                      */\n"
        "/* ═══════════════════════════════════════════════════════════════════ */\n"
        "\n"
        "QTreeWidget {\n"
        "    background-color: #FFFFFF;\n"
        "    alternate-background-color: #F8F8F8;\n"
        "    selection-background-color: %1;\n"
        "    selection-color: #000000;\n"
        "    border: 1px solid #E0E0E0;\n"
        "    border-radius: 3px;\n"
        "    outline: none;\n"
        "    font-family: '%2';\n"
        "    font-size: %3px;\n"
        "    show-decoration-selected: 1;\n"
        "}\n"
        "\n"
        "QTreeWidget::item {\n"
        "    padding: 4px 8px;\n"
        "    border-bottom: 1px solid #F0F0F0;\n"
        "    height: 28px;\n"
        "}\n"
        "\n"
        "QTreeWidget::item:selected {\n"
        "    background-color: %4;\n"
        "    color: #000000;\n"
        "    border: 1px solid %5;\n"
        "    border-radius: 2px;\n"
        "}\n"
        "\n"
        "QTreeWidget::item:hover {\n"
        "    background-color: #E8F4FF;\n"
        "}\n"
        "\n"
        "/* Service Type Color Coding */\n"
        "QTreeWidget::item[serviceType=\"audio\"] {\n"
        "    color: %6;\n"
        "    font-weight: bold;\n"
        "}\n"
        "\n"
        "QTreeWidget::item[serviceType=\"data\"] {\n"
        "    color: %7;\n"
        "    font-weight: bold;\n"
        "}\n"
        "\n"
        "QTreeWidget::item[serviceType=\"ensemble\"] {\n"
        "    color: %8;\n"
        "    font-weight: bold;\n"
        "    font-size: %9px;\n"
        "}\n"
        "\n"
        "QTreeWidget::item[serviceType=\"packet\"] {\n"
        "    color: %10;\n"
        "    font-style: italic;\n"
        "}\n"
        "\n"
        "/* Tree Branch Styling */\n"
        "QTreeWidget::branch:has-siblings:!adjoins-item {\n"
        "    border-image: none;\n"
        "    border-left: 1px dotted #CCCCCC;\n"
        "}\n"
        "\n"
        "QTreeWidget::branch:has-siblings:adjoins-item {\n"
        "    border-image: none;\n"
        "    border-left: 1px dotted #CCCCCC;\n"
        "    border-top: 1px dotted #CCCCCC;\n"
        "}\n"
        "\n"
        "QTreeWidget::branch:!has-children:!has-siblings:adjoins-item {\n"
        "    border-image: none;\n"
        "    border-left: 1px dotted #CCCCCC;\n"
        "    border-top: 1px dotted #CCCCCC;\n"
        "}\n"
        "\n"
        "QTreeWidget::branch:has-children:!has-siblings:closed,\n"
        "QTreeWidget::branch:closed:has-children:has-siblings {\n"
        "    border-image: none;\n"
        "    image: none;\n"
        "    border-left: 1px dotted #CCCCCC;\n"
        "}\n"
        "\n"
        "QTreeWidget::branch:open:has-children:!has-siblings,\n"
        "QTreeWidget::branch:open:has-children:has-siblings {\n"
        "    border-image: none;\n"
        "    image: none;\n"
        "    border-left: 1px dotted #CCCCCC;\n"
        "}\n"
    ).arg(
        colorToHex(ACCENT_BLUE_LIGHT),       // %1 - Selection background
        FONT_FAMILY_UI,                      // %2 - Font family
        QString::number(FONT_SIZE_UI),       // %3 - Font size
        colorToHex(ACCENT_BLUE_LIGHT),       // %4 - Selected item background
        colorToHex(ACCENT_BLUE),             // %5 - Selected item border
        colorToHex(SERVICE_AUDIO),           // %6 - Audio service color
        colorToHex(SERVICE_DATA),            // %7 - Data service color
        colorToHex(SERVICE_ENSEMBLE),        // %8 - Ensemble color
        QString::number(FONT_SIZE_UI + 1),   // %9 - Ensemble font size
        colorToHex(SERVICE_PACKET)           // %10 - Packet service color
    );
}

QString BroadcastTheme::getFrameTableStyleSheet()
{
    return QString(
        "/* ═══════════════════════════════════════════════════════════════════ */\n"
        "/* ETI FRAME LISTING TABLE WITH FRAME TYPE COLOR CODING                 */\n"
        "/* ═══════════════════════════════════════════════════════════════════ */\n"
        "\n"
        "QTableWidget {\n"
        "    background-color: #FFFFFF;\n"
        "    alternate-background-color: #F8F8F8;\n"
        "    selection-background-color: %1;\n"
        "    selection-color: #000000;\n"
        "    gridline-color: #E8E8E8;\n"
        "    border: 1px solid #CCCCCC;\n"
        "    border-radius: 3px;\n"
        "    outline: none;\n"
        "    font-family: '%2';\n"
        "    font-size: %3px;\n"
        "}\n"
        "\n"
        "QTableWidget::item {\n"
        "    padding: 4px 6px;\n"
        "    border-bottom: 1px solid #F0F0F0;\n"
        "    height: 24px;\n"
        "}\n"
        "\n"
        "QTableWidget::item:selected {\n"
        "    background-color: %4;\n"
        "    color: #000000;\n"
        "}\n"
        "\n"
        "QTableWidget::item:hover {\n"
        "    background-color: #E8F4FF;\n"
        "}\n"
        "\n"
        "/* Frame Type Color Coding */\n"
        "QTableWidget::item[frameType=\"FIC\"] {\n"
        "    background-color: %5;\n"
        "    color: #FFFFFF;\n"
        "}\n"
        "\n"
        "QTableWidget::item[frameType=\"MSC\"] {\n"
        "    background-color: %6;\n"
        "    color: #FFFFFF;\n"
        "}\n"
        "\n"
        "QTableWidget::item[frameType=\"SYNC\"] {\n"
        "    background-color: %7;\n"
        "    color: #FFFFFF;\n"
        "}\n"
        "\n"
        "QTableWidget::item[frameType=\"ERROR\"] {\n"
        "    background-color: %8;\n"
        "    color: #FFFFFF;\n"
        "    font-weight: bold;\n"
        "}\n"
        "\n"
        "/* Quality Status Color Coding */\n"
        "QTableWidget::item[quality=\"excellent\"] {\n"
        "    border-left: 4px solid %9;\n"
        "}\n"
        "\n"
        "QTableWidget::item[quality=\"good\"] {\n"
        "    border-left: 4px solid %10;\n"
        "}\n"
        "\n"
        "QTableWidget::item[quality=\"fair\"] {\n"
        "    border-left: 4px solid %11;\n"
        "}\n"
        "\n"
        "QTableWidget::item[quality=\"poor\"] {\n"
        "    border-left: 4px solid %12;\n"
        "}\n"
        "\n"
        "QTableWidget::item[quality=\"bad\"] {\n"
        "    border-left: 4px solid %13;\n"
        "}\n"
        "\n"
        "/* Professional Header Styling */\n"
        "QHeaderView::section {\n"
        "    background: %14;\n"
        "    border: 1px solid #CCCCCC;\n"
        "    padding: 6px 8px;\n"
        "    font-weight: bold;\n"
        "    font-size: %15px;\n"
        "    color: #333333;\n"
        "    text-align: center;\n"
        "}\n"
        "\n"
        "QHeaderView::section:hover {\n"
        "    background: %16;\n"
        "}\n"
    ).arg(
        colorToHex(ACCENT_BLUE_LIGHT),                                   // %1 - Selection background
        FONT_FAMILY_MONO,                                                // %2 - Monospace font for technical data
        QString::number(FONT_SIZE_TECHNICAL),                            // %3 - Technical font size
        colorToHex(ACCENT_BLUE_LIGHT),                                   // %4 - Selected item background
        colorToHex(withAlpha(FRAME_FIC, 0.8)),                          // %5 - FIC frame background
        colorToHex(withAlpha(FRAME_MSC, 0.8)),                          // %6 - MSC frame background
        colorToHex(withAlpha(FRAME_SYNC, 0.8)),                         // %7 - SYNC frame background
        colorToHex(withAlpha(FRAME_ERROR, 0.8)),                        // %8 - ERROR frame background
        colorToHex(QUALITY_EXCELLENT),                                   // %9 - Excellent quality
        colorToHex(QUALITY_GOOD),                                        // %10 - Good quality
        colorToHex(QUALITY_FAIR),                                        // %11 - Fair quality
        colorToHex(QUALITY_POOR),                                        // %12 - Poor quality
        colorToHex(QUALITY_BAD),                                         // %13 - Bad quality
        createGradient(QColor("#F8F8F8"), QColor("#E8E8E8")),           // %14 - Header background
        QString::number(FONT_SIZE_SMALL),                                // %15 - Header font size
        createGradient(QColor("#E8F4FF"), QColor("#CCE8FF"))            // %16 - Header hover background
    );
}

QString BroadcastTheme::getPropertiesPanelStyleSheet()
{
    return QString(
        "/* ═══════════════════════════════════════════════════════════════════ */\n"
        "/* PROPERTIES PANEL WITH PROFESSIONAL PARAMETER DISPLAY                 */\n"
        "/* ═══════════════════════════════════════════════════════════════════ */\n"
        "\n"
        "QWidget#PropertiesPanel {\n"
        "    background-color: %1;\n"
        "    border: 1px solid #E0E0E0;\n"
        "    border-radius: 3px;\n"
        "}\n"
        "\n"
        "/* Parameter Display Areas */\n"
        "QTextEdit#ProfessionalParametersText {\n"
        "    background-color: #FAFAFA;\n"
        "    border: 1px solid #E0E0E0;\n"
        "    border-radius: 3px;\n"
        "    font-family: '%2';\n"
        "    font-size: %3px;\n"
        "    line-height: 1.4;\n"
        "    padding: 8px;\n"
        "}\n"
        "\n"
        "QTextEdit#ProfessionalYamlOutputText {\n"
        "    background-color: #FFFEF8;\n"
        "    border: 1px solid #E0E0E0;\n"
        "    border-radius: 3px;\n"
        "    font-family: '%4';\n"
        "    font-size: %5px;\n"
        "    line-height: 1.4;\n"
        "    padding: 8px;\n"
        "}\n"
        "\n"
        "/* Quick Stats Frame */\n"
        "QFrame#QuickStatsFrame {\n"
        "    background-color: #F0F6FF;\n"
        "    border: 1px solid %6;\n"
        "    border-radius: 3px;\n"
        "    padding: 8px;\n"
        "}\n"
        "\n"
        "QLabel#StatLabel {\n"
        "    font-size: %7px;\n"
        "    color: #666666;\n"
        "    font-weight: bold;\n"
        "}\n"
        "\n"
        "QLabel#StatValue {\n"
        "    font-size: %8px;\n"
        "    font-family: '%9';\n"
        "    color: #333333;\n"
        "    font-weight: bold;\n"
        "}\n"
        "\n"
        "/* Mode Toggle Buttons */\n"
        "QPushButton#SyncModeBtn {\n"
        "    background: %10;\n"
        "    border: 2px solid %11;\n"
        "    border-radius: 4px;\n"
        "    padding: 6px 12px;\n"
        "    color: %12;\n"
        "    font-weight: bold;\n"
        "}\n"
        "\n"
        "QPushButton#SyncModeBtn:checked {\n"
        "    background: %13;\n"
        "    color: #FFFFFF;\n"
        "    border-color: %14;\n"
        "}\n"
        "\n"
        "QPushButton#CompareModeBtn {\n"
        "    background: %15;\n"
        "    border: 2px solid %16;\n"
        "    border-radius: 4px;\n"
        "    padding: 6px 12px;\n"
        "    color: %17;\n"
        "    font-weight: bold;\n"
        "}\n"
        "\n"
        "QPushButton#CompareModeBtn:checked {\n"
        "    background: %18;\n"
        "    color: #FFFFFF;\n"
        "    border-color: %19;\n"
        "}\n"
    ).arg(
        colorToHex(BACKGROUND_LIGHT),                                    // %1 - Panel background
        FONT_FAMILY_MONO,                                                // %2 - Parameters text font
        QString::number(FONT_SIZE_TECHNICAL),                            // %3 - Parameters text size
        FONT_FAMILY_MONO,                                                // %4 - YAML text font
        QString::number(FONT_SIZE_TECHNICAL),                            // %5 - YAML text size
        colorToHex(ACCENT_BLUE_LIGHT),                                   // %6 - Stats frame border
        QString::number(FONT_SIZE_SMALL),                                // %7 - Stat label size
        QString::number(FONT_SIZE_TECHNICAL),                            // %8 - Stat value size
        FONT_FAMILY_MONO,                                                // %9 - Stat value font
        createGradient(QColor("#E8F4FF"), QColor("#CCE8FF")),           // %10 - Sync button background
        colorToHex(ACCENT_BLUE),                                         // %11 - Sync button border
        colorToHex(ACCENT_BLUE),                                         // %12 - Sync button text
        createGradient(ACCENT_BLUE, ACCENT_BLUE_DARK),                   // %13 - Sync button checked background
        colorToHex(ACCENT_BLUE_DARK),                                    // %14 - Sync button checked border
        createGradient(QColor("#FFF8E1"), QColor("#FFECB3")),           // %15 - Compare button background
        colorToHex(QColor("#FF8F00")),                                   // %16 - Compare button border
        colorToHex(QColor("#FF8F00")),                                   // %17 - Compare button text
        createGradient(QColor("#FF8F00"), QColor("#E65100")),           // %18 - Compare button checked background
        colorToHex(QColor("#E65100"))                                    // %19 - Compare button checked border
    );
}

QString BroadcastTheme::getBottomPanelsStyleSheet()
{
    return QString(
        "/* ═══════════════════════════════════════════════════════════════════ */\n"
        "/* BOTTOM TOOL PANELS - SPECIALIZED BROADCAST ANALYSIS TOOLS            */\n"
        "/* ═══════════════════════════════════════════════════════════════════ */\n"
        "\n"
        "QTabWidget#ProfessionalBottomToolTabs {\n"
        "    background-color: %1;\n"
        "    border: 1px solid #CCCCCC;\n"
        "    border-radius: 4px;\n"
        "}\n"
        "\n"
        "/* Hex Viewer Styling */\n"
        "QTextEdit#ProfessionalHexViewer {\n"
        "    background-color: #F8F8F8;\n"
        "    border: 1px solid #E0E0E0;\n"
        "    border-radius: 3px;\n"
        "    font-family: '%2';\n"
        "    font-size: %3px;\n"
        "    line-height: 1.2;\n"
        "    color: #333333;\n"
        "}\n"
        "\n"
        "/* Messages Log Styling */\n"
        "QTextEdit#ProfessionalMessagesLog {\n"
        "    background-color: #FFFFFF;\n"
        "    border: 1px solid #E0E0E0;\n"
        "    border-radius: 3px;\n"
        "    font-family: '%4';\n"
        "    font-size: %5px;\n"
        "    color: #333333;\n"
        "}\n"
        "\n"
        "/* Audio Monitor Styling */\n"
        "QWidget#AudioMonitorWidget {\n"
        "    background-color: #F0F0F0;\n"
        "    border: 1px solid #E0E0E0;\n"
        "    border-radius: 3px;\n"
        "}\n"
        "\n"
        "/* TR 101-290 Analysis Styling */\n"
        "QTextEdit#TR101290Analysis {\n"
        "    background-color: #FFF8F0;\n"
        "    border: 1px solid #E0E0E0;\n"
        "    border-radius: 3px;\n"
        "    font-family: '%6';\n"
        "    font-size: %7px;\n"
        "    color: #333333;\n"
        "}\n"
        "\n"
        "/* Comments and EPG Styling */\n"
        "QTextEdit#ProfessionalCommentsText {\n"
        "    background-color: #FFFEF8;\n"
        "    border: 1px solid #E0E0E0;\n"
        "    border-radius: 3px;\n"
        "    font-family: '%8';\n"
        "    font-size: %9px;\n"
        "    color: #333333;\n"
        "}\n"
        "\n"
        "QTextEdit#ProfessionalEPGViewer {\n"
        "    background-color: #F8F8FF;\n"
        "    border: 1px solid #E0E0E0;\n"
        "    border-radius: 3px;\n"
        "    font-family: '%10';\n"
        "    font-size: %11px;\n"
        "    color: #333333;\n"
        "}\n"
        "\n"
        "/* Control Panel Styling */\n"
        "QFrame#ControlPanel {\n"
        "    background-color: #F8F8F8;\n"
        "    border: 1px solid #E0E0E0;\n"
        "    border-radius: 3px;\n"
        "    padding: 6px;\n"
        "}\n"
    ).arg(
        colorToHex(BACKGROUND_LIGHT),        // %1 - Tab widget background
        FONT_FAMILY_MONO,                    // %2 - Hex viewer font
        QString::number(FONT_SIZE_SMALL),    // %3 - Hex viewer font size
        FONT_FAMILY_MONO,                    // %4 - Messages log font
        QString::number(FONT_SIZE_TECHNICAL), // %5 - Messages log font size
        FONT_FAMILY_MONO,                    // %6 - TR 101-290 font
        QString::number(FONT_SIZE_TECHNICAL), // %7 - TR 101-290 font size
        FONT_FAMILY_UI,                      // %8 - Comments font
        QString::number(FONT_SIZE_UI),       // %9 - Comments font size
        FONT_FAMILY_UI,                      // %10 - EPG font
        QString::number(FONT_SIZE_UI)        // %11 - EPG font size
    );
}

// ═══════════════════════════════════════════════════════════════════
// SERVICE TYPE AND STATUS COLOR MAPPING METHODS
// ═══════════════════════════════════════════════════════════════════

QColor BroadcastTheme::getServiceTypeColor(int serviceType)
{
    switch (serviceType) {
        case 0: return SERVICE_AUDIO;      // Audio service
        case 1: return SERVICE_DATA;       // Data service
        case 2: return SERVICE_PACKET;     // Packet mode service
        case 3: return SERVICE_ENSEMBLE;   // Ensemble information
        default: return SERVICE_UNKNOWN;   // Unknown service type
    }
}

QColor BroadcastTheme::getQualityColor(double qualityPercent)
{
    if (qualityPercent >= 95.0) return QUALITY_EXCELLENT;
    else if (qualityPercent >= 85.0) return QUALITY_GOOD;
    else if (qualityPercent >= 70.0) return QUALITY_FAIR;
    else if (qualityPercent >= 50.0) return QUALITY_POOR;
    else return QUALITY_BAD;
}

QColor BroadcastTheme::getFrameStatusColor(bool hasErrors, const QString& frameType)
{
    if (hasErrors) return FRAME_ERROR;
    
    if (frameType.contains("FIC", Qt::CaseInsensitive)) return FRAME_FIC;
    else if (frameType.contains("MSC", Qt::CaseInsensitive)) return FRAME_MSC;
    else if (frameType.contains("SYNC", Qt::CaseInsensitive)) return FRAME_SYNC;
    else return FRAME_PADDING;
}

QColor BroadcastTheme::getFigTypeColor(int figType)
{
    switch (figType) {
        case 0: return QColor("#2196F3");    // Ensemble configuration (Blue)
        case 1: return QColor("#4CAF50");    // Service information (Green)
        case 2: return QColor("#FF9800");    // Service component (Orange)
        case 3: return QColor("#9C27B0");    // Service component data (Purple)
        case 4: return QColor("#F44336");    // Service component global definition (Red)
        case 5: return QColor("#607D8B");    // Data service component (Blue Gray)
        case 6: return QColor("#795548");    // Conditional access (Brown)
        case 7: return QColor("#009688");    // Configuration information (Teal)
        default: return QColor("#9E9E9E");   // Other/Unknown (Gray)
    }
}

// ═══════════════════════════════════════════════════════════════════
// PROFESSIONAL UI HELPER METHODS
// ═══════════════════════════════════════════════════════════════════

QString BroadcastTheme::createButtonStyle(const QColor& baseColor, const QColor& textColor)
{
    return QString(
        "background: %1; "
        "border: 1px solid %2; "
        "border-radius: 4px; "
        "padding: 6px 12px; "
        "color: %3; "
        "font-family: '%4'; "
        "font-size: %5px;"
    ).arg(
        createGradient(baseColor.lighter(110), baseColor),
        colorToHex(baseColor.darker(120)),
        colorToHex(textColor),
        FONT_FAMILY_UI,
        QString::number(FONT_SIZE_UI)
    );
}

QString BroadcastTheme::createPanelHeaderStyle(const QString& title, const QColor& backgroundColor)
{
    Q_UNUSED(title) // Title could be used for custom styling per panel
    return QString(
        "font-family: '%1'; "
        "font-weight: bold; "
        "font-size: %2px; "
        "color: #FFFFFF; "
        "padding: 8px 12px; "
        "background: %3; "
        "border: 1px solid %4; "
        "border-radius: 4px; "
        "margin-bottom: 2px;"
    ).arg(
        FONT_FAMILY_HEADER,
        QString::number(FONT_SIZE_HEADER),
        createGradient(backgroundColor, backgroundColor.darker(110)),
        colorToHex(backgroundColor.darker(130))
    );
}

QString BroadcastTheme::createStatusLedStyle(bool isActive, const QColor& statusColor)
{
    QColor ledColor = isActive ? statusColor : QColor("#D0D0D0");
    return QString(
        "background-color: %1; "
        "border: 1px solid %2; "
        "border-radius: 6px; "
        "width: 12px; "
        "height: 12px;"
    ).arg(
        colorToHex(ledColor),
        colorToHex(ledColor.darker(150))
    );
}

// ═══════════════════════════════════════════════════════════════════
// COLOR UTILITY METHODS
// ═══════════════════════════════════════════════════════════════════

QString BroadcastTheme::colorToHex(const QColor& color)
{
    return QString("#%1%2%3")
        .arg(color.red(), 2, 16, QChar('0'))
        .arg(color.green(), 2, 16, QChar('0'))
        .arg(color.blue(), 2, 16, QChar('0'));
}

QString BroadcastTheme::createGradient(const QColor& startColor, const QColor& endColor, const QString& direction)
{
    QString gradientDirection = (direction == "horizontal") ? "x1:0, y1:0, x2:1, y2:0" : "x1:0, y1:0, x2:0, y2:1";
    return QString("qlineargradient(%1, stop:0 %2, stop:1 %3)")
        .arg(gradientDirection, colorToHex(startColor), colorToHex(endColor));
}

QColor BroadcastTheme::withAlpha(const QColor& color, double alpha)
{
    QColor result = color;
    result.setAlphaF(alpha);
    return result;
}