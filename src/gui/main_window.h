/**
 * @file main_window.h
 * @brief Forward declaration header for main application window (GUI testing support)
 * 
 * This header provides access to DABAnalyzerWindow class for GUI testing.
 * The actual class implementation is in src/main.cpp.
 * 
 * @note This is a minimal header created specifically to enable GUI automated testing.
 *       The full DABAnalyzerWindow class (3000+ lines) remains in main.cpp.
 * 
 * @author StreamDAB Analyzer Project
 * @date 2025-10-23
 */

#ifndef MAIN_WINDOW_H
#define MAIN_WINDOW_H

#include <QMainWindow>
#include <QWidget>

class QTreeWidget;
class QListWidget;
class QTextEdit;
class QLabel;
class QProgressBar;
class QMenuBar;
class QStatusBar;
class QDockWidget;

class EnhancedETIProcessorQt;
class AdvancedFIGAnalyzer;
class HexViewerFormatter;

namespace etsi {
    class ETSIComplianceEngine;
}

namespace multistream {
    class MultiStreamProcessor;
}

namespace error_detection {
    class AdvancedErrorDetector;
}

namespace eti::dlsplus {
    class DLSPlusDecoder;
}

/**
 * @class DABAnalyzerWindow
 * @brief Main application window for DAB/ETI stream analysis
 * 
 * Professional three-panel broadcast analysis interface with:
 * - ETI frame processing and visualization
 * - FIG analysis and service discovery
 * - Real-time performance monitoring
 * - ETSI compliance validation
 * - DLS+ "Now Playing" metadata display
 * 
 * @note Full implementation in src/main.cpp (3000+ lines)
 *       This header enables GUI automated testing
 */
class DABAnalyzerWindow : public QMainWindow
{
    Q_OBJECT

public:
    /**
     * @brief Construct main analyzer window
     * @param parent Parent widget (typically nullptr for main window)
     */
    explicit DABAnalyzerWindow(QWidget *parent = nullptr);
    
    /**
     * @brief Destructor - clean shutdown of all processing
     */
    ~DABAnalyzerWindow() override;

    // Disable copy/move
    DABAnalyzerWindow(const DABAnalyzerWindow&) = delete;
    DABAnalyzerWindow& operator=(const DABAnalyzerWindow&) = delete;
    DABAnalyzerWindow(DABAnalyzerWindow&&) = delete;
    DABAnalyzerWindow& operator=(DABAnalyzerWindow&&) = delete;

public:
    /**
     * @brief Access methods for testing
     * @{
     */
    QMenuBar* getMenuBar() const { return menuBar(); }
    QStatusBar* getStatusBar() const { return statusBar(); }
    
    // Note: Additional test accessors can be added here as needed
    // Full implementation provides comprehensive UI access
    /** @} */
};

// Type alias for compatibility with older test code expecting "MainWindow"
using MainWindow = DABAnalyzerWindow;

#endif // MAIN_WINDOW_H
