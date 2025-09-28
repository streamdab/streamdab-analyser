#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QMenuBar>
#include <QToolBar>
#include <QStatusBar>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QLabel>
#include <QPushButton>
#include <QProgressBar>
#include <QTextEdit>
#include <QGroupBox>
#include <QFileDialog>
#include <QTimer>
#include <QApplication>

#include "enhanced_eti_processor_qt.h"

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    // File operations
    void openETIFile();
    void onRecentFileTriggered();
    
    // ETI Processing slots
    void onProcessingStarted(const QString& fileName, int estimatedFrames);
    void onFrameProcessed(const ProcessedFrame& frame);
    void onProcessingProgress(int percentage, int currentFrame, int totalFrames);
    void onProcessingComplete(int totalFrames, int processingTimeMs);
    void onProcessingError(const QString& error);
    void onFormatDetected(ETIFormat format, const QString& formatName);
    void onFigDiscovered(const FIGInfo& fig);

private:
    // UI Setup methods
    void setupUI();
    void setupMenuBar();
    void setupToolBar();
    void setupStatusBar();
    void setupCentralWidget();
    void applyProfessionalTheme();
    
    // Update methods
    void updateFrameDisplay(const ProcessedFrame& frame);
    void updateStatistics();
    void clearDisplay();
    
    // Menu and toolbar
    QAction* m_openAction;
    QAction* m_exitAction;
    
    // Main layout widgets
    QWidget* m_centralWidget;
    QVBoxLayout* m_mainLayout;
    
    // File operations panel
    QGroupBox* m_fileGroup;
    QHBoxLayout* m_fileLayout;
    QPushButton* m_openButton;
    QLabel* m_fileLabel;
    
    // Processing status panel
    QGroupBox* m_statusGroup;
    QGridLayout* m_statusLayout;
    QLabel* m_statusLabel;
    QProgressBar* m_progressBar;
    QLabel* m_frameCountLabel;
    QLabel* m_formatLabel;
    QLabel* m_processingTimeLabel;
    
    // Frame analysis panel
    QGroupBox* m_frameGroup;
    QGridLayout* m_frameLayout;
    QLabel* m_currentFrameLabel;
    QLabel* m_syncPatternLabel;
    QLabel* m_lidataLabel;
    QLabel* m_crcLabel;
    QLabel* m_ficSizeLabel;
    QLabel* m_mscSizeLabel;
    
    // Statistics panel
    QGroupBox* m_statsGroup;
    QGridLayout* m_statsLayout;
    QLabel* m_totalFramesLabel;
    QLabel* m_etiNICountLabel;
    QLabel* m_etiLIACountLabel;
    QLabel* m_etiLIBCountLabel;
    QLabel* m_processingRateLabel;
    
    // Processing log
    QGroupBox* m_logGroup;
    QVBoxLayout* m_logLayout;
    QTextEdit* m_logDisplay;
    
    // Status bar
    QLabel* m_statusBarLabel;
    QLabel* m_versionLabel;
    
    // ETI Processor
    EnhancedETIProcessorQt* m_etiProcessor;
    
    // Current state
    QString m_currentFileName;
    int m_totalFramesExpected;
    QDateTime m_processingStartTime;
    
    // Statistics tracking
    int m_totalFramesProcessed;
    int m_figCount;
};

#endif // MAINWINDOW_H