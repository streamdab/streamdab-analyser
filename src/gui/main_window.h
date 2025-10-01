#pragma once

#include <QApplication>
#include <QMainWindow>
#include <QCloseEvent>
#include <QCoreApplication>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QSplitter>
#include <QTreeWidget>
#include <QListWidget>
#include <QTextEdit>
#include <QLabel>
#include <QProgressBar>
#include <QStatusBar>
#include <QMenuBar>
#include <QFileDialog>
#include <QMessageBox>
#include <QHeaderView>
#include <QGroupBox>
#include <QCheckBox>
#include <QTabWidget>
#include <QTimer>
#include <QComboBox>
#include <QSpinBox>
#include <QPushButton>
#include <QtConcurrent>
#include <QFuture>
#include <QFutureWatcher>
#include <chrono>
#include <memory>

#include "core/enhanced_eti_processor_qt.h"
#include "core/advanced_fig_analyzer.h"
#include "core/etsi_compliance_engine.hpp"
#include "core/multi_stream_processor.hpp"
#include "core/advanced_error_detection.hpp"
#include "core/professional_logging_system.hpp"

class Phase14DABAnalyzerWindow : public QMainWindow
{
    Q_OBJECT

public:
    Phase14DABAnalyzerWindow(QWidget *parent = nullptr);
    
private slots:
    void openFile();
    void onSingleFrameProcessed(const ProcessedFrame& frame);
    void onProgressUpdated(int percentage, int currentFrame, int totalFrames);
    void onProcessingCompleted();
    void onErrorDetected();

private:
    void initializeSimpleComponents();
    void ensureETIProcessorReady();
    void setupUI();
    void setupMenuBar();
    void setupSignalConnections();
    void setupStatusBar();
    void createEnsemblePanel(QSplitter* parent);
    void createFrameNavigatorPanel(QSplitter* parent);
    void createServiceDetailsPanel(QSplitter* parent);
    void createAnalysisTabsPanel(QSplitter* parent);
    void displayFrameHexData(int frameIndex);
    QWidget* createEnsembleAnalysisTab();
    QWidget* createFICAnalysisTab();
    QWidget* createServiceAnalysisTab();
    QWidget* createErrorAnalysisTab();
    QWidget* createAudioAnalysisTab();

    // Core processors
    std::unique_ptr<EnhancedETIProcessorQt> m_etiProcessor;
    std::unique_ptr<AdvancedFIGAnalyzer> m_figAnalyzer;
    std::unique_ptr<etsi::ETSIComplianceEngine> m_etsiComplianceEngine;
    std::unique_ptr<multistream::MultiStreamProcessor> m_multiStreamProcessor;
    std::unique_ptr<error_detection::AdvancedErrorDetector> m_errorDetector;
    std::unique_ptr<professional_logging::ProfessionalLoggingSystem> m_loggingSystem;

    // UI components
    QSplitter* m_mainSplitter;
    QTreeWidget* m_serviceTree;
    QTabWidget* m_analysisTabs;
    QTextEdit* m_propertiesPanel;
    QListWidget* m_frameList;
    QProgressBar* m_progressBar;
    QLabel* m_statusLabel;
};