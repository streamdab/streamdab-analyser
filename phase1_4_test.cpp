#include <QApplication>
#include <QMainWindow>
#include <QVBoxLayout>
#include <QLabel>
#include <QProgressBar>
#include <QTimer>
#include <QDebug>
#include <QFileInfo>
#include <QHBoxLayout>
#include <QSplashScreen>
#include <QPixmap>
#include <QThread>

// Include our fixed components
#include "src/core/enhanced_eti_processor_qt.h"
#include "src/core/advanced_fig_analyzer.h"

class Phase14TestWindow : public QMainWindow
{
    Q_OBJECT

public:
    Phase14TestWindow(QWidget *parent = nullptr) : QMainWindow(parent)
    {
        setWindowTitle("Phase 1.4 Test - DAB Analyzer Frame Processing & FIG Analysis");
        setMinimumSize(800, 600);
        
        // Create central widget
        QWidget* centralWidget = new QWidget();
        setCentralWidget(centralWidget);
        
        QVBoxLayout* layout = new QVBoxLayout(centralWidget);
        
        // Title
        QLabel* title = new QLabel("StreamDAB Stream Analyzer - Phase 1.4 Test");
        title->setStyleSheet("font-size: 18px; font-weight: bold; color: #0066CC; margin: 10px;");
        layout->addWidget(title);
        
        // Processing info
        m_processingLabel = new QLabel("Ready to process ETI file...");
        layout->addWidget(m_processingLabel);
        
        // Progress bar
        m_progressBar = new QProgressBar();
        m_progressBar->setVisible(false);
        layout->addWidget(m_progressBar);
        
        // Frame count
        m_frameCountLabel = new QLabel("Frames processed: 0");
        layout->addWidget(m_frameCountLabel);
        
        // FIG Analysis results
        QLabel* figLabel = new QLabel("FIG Analysis Results:");
        figLabel->setStyleSheet("font-weight: bold; margin-top: 20px;");
        layout->addWidget(figLabel);
        
        m_figResultsLabel = new QLabel("No FIG data processed yet...");
        m_figResultsLabel->setWordWrap(true);
        m_figResultsLabel->setStyleSheet("background-color: #f5f5f5; padding: 10px; border: 1px solid #ccc;");
        layout->addWidget(m_figResultsLabel);
        
        // Service discovery results
        QLabel* serviceLabel = new QLabel("Service Discovery Results:");
        serviceLabel->setStyleSheet("font-weight: bold; margin-top: 20px;");
        layout->addWidget(serviceLabel);
        
        m_serviceResultsLabel = new QLabel("No services discovered yet...");
        m_serviceResultsLabel->setWordWrap(true);
        m_serviceResultsLabel->setStyleSheet("background-color: #f5f5f5; padding: 10px; border: 1px solid #ccc;");
        layout->addWidget(m_serviceResultsLabel);
        
        // Initialize components
        m_etiProcessor = std::make_unique<EnhancedETIProcessorQt>();
        m_figAnalyzer = std::make_unique<AdvancedFIGAnalyzer>();
        
        // Connect signals
        setupSignalConnections();
        
        // Auto-process Bangkok file if available
        QTimer::singleShot(1000, this, &Phase14TestWindow::autoProcessBangkokFile);
    }

private slots:
    void onFrameProcessed(const ProcessedFrame& frame)
    {
        m_frameCount++;
        m_frameCountLabel->setText(QString("Frames processed: %1").arg(m_frameCount));
        
        // Update processing label
        m_processingLabel->setText(QString("Processing frame #%1 - Format: %2")
                                  .arg(frame.frame_number)
                                  .arg(formatToString(frame.format)));
        
        // Feed FIC data to FIG analyzer
        if (!frame.fic_data.isEmpty()) {
            m_figAnalyzer->analyzeFICData(frame.fic_data);
        }
        
        // Update progress
        if (m_totalFrames > 0) {
            int progress = (m_frameCount * 100) / m_totalFrames;
            m_progressBar->setValue(progress);
        }
    }
    
    void onProcessingComplete(int totalFrames, int processingTimeMs)
    {
        m_processingLabel->setText(QString("✅ Processing Complete! %1 frames in %2ms")
                                  .arg(totalFrames)
                                  .arg(processingTimeMs));
        m_progressBar->setVisible(false);
        
        // Show FIG analysis summary
        updateFIGAnalysisSummary();
    }
    
    void onServiceDiscovered(const ServiceInfo& service)
    {
        m_discoveredServices.append(service);
        updateServiceDiscoverySummary();
    }
    
    void onFIGProcessed(const FIGData& figData)
    {
        m_processedFIGs.append(figData);
        updateFIGAnalysisSummary();
    }
    
    void autoProcessBangkokFile()
    {
        QString filePath = "/home/seksan/workspace/streamdab-analyzer/eti/bkk_20062022_141637.eti";
        QFileInfo fileInfo(filePath);
        
        if (!fileInfo.exists()) {
            m_processingLabel->setText("❌ Bangkok ETI file not found at: " + filePath);
            return;
        }
        
        // Calculate expected frames
        qint64 fileSize = fileInfo.size();
        m_totalFrames = fileSize / 6144;
        
        m_processingLabel->setText(QString("🔄 Loading Bangkok ETI file (%1 MB, ~%2 frames)...")
                                  .arg(fileSize / (1024 * 1024))
                                  .arg(m_totalFrames));
        
        m_progressBar->setVisible(true);
        m_progressBar->setRange(0, 100);
        m_progressBar->setValue(0);
        
        // Start processing
        bool success = m_etiProcessor->processETIFile(filePath);
        if (!success) {
            m_processingLabel->setText("❌ Failed to start processing Bangkok ETI file");
            m_progressBar->setVisible(false);
        }
    }

private:
    void setupSignalConnections()
    {
        // ETI Processor connections
        connect(m_etiProcessor.get(), &EnhancedETIProcessorQt::frameProcessed,
                this, &Phase14TestWindow::onFrameProcessed);
        connect(m_etiProcessor.get(), &EnhancedETIProcessorQt::processingComplete,
                this, &Phase14TestWindow::onProcessingComplete);
        
        // FIG Analyzer connections
        connect(m_figAnalyzer.get(), &AdvancedFIGAnalyzer::serviceDiscovered,
                this, &Phase14TestWindow::onServiceDiscovered);
        connect(m_figAnalyzer.get(), &AdvancedFIGAnalyzer::figProcessed,
                this, &Phase14TestWindow::onFIGProcessed);
    }
    
    void updateFIGAnalysisSummary()
    {
        QString summary;
        summary += QString("📊 FIG Analysis Summary:\\n");
        summary += QString("Total FIGs processed: %1\\n").arg(m_processedFIGs.size());
        
        // Count FIG types
        QMap<QString, int> figTypeCounts;
        for (const auto& fig : m_processedFIGs) {
            QString figKey = QString("%1/%2").arg(fig.figType).arg(fig.figExtension);
            figTypeCounts[figKey]++;
        }
        
        summary += "FIG Types found:\\n";
        for (auto it = figTypeCounts.begin(); it != figTypeCounts.end(); ++it) {
            summary += QString("  • FIG %1: %2 instances\\n").arg(it.key()).arg(it.value());
        }
        
        m_figResultsLabel->setText(summary);
    }
    
    void updateServiceDiscoverySummary()
    {
        QString summary;
        summary += QString("🎵 Service Discovery Summary:\\n");
        summary += QString("Total services discovered: %1\\n\\n").arg(m_discoveredServices.size());
        
        for (const auto& service : m_discoveredServices) {
            summary += QString("• %1 (SID: 0x%2)\\n")
                      .arg(service.label.isEmpty() ? QString("Service 0x%1").arg(service.serviceId, 4, 16, QChar('0')) : service.label)
                      .arg(service.serviceId, 4, 16, QChar('0'));
            summary += QString("  Type: %1\\n").arg(service.type);
            if (service.isAudio) summary += "  Audio Service\\n";
            if (service.isData) summary += "  Data Service\\n";
            summary += "\\n";
        }
        
        m_serviceResultsLabel->setText(summary);
    }
    
    QString formatToString(ETIFormat format)
    {
        switch (format) {
            case ETIFormat::ETI_NI: return "ETI-NI";
            case ETIFormat::ETI_LI_A: return "ETI-LI-A (Bangkok Pattern A)";
            case ETIFormat::ETI_LI_B: return "ETI-LI-B (Bangkok Pattern B)";
            default: return "Unknown";
        }
    }

private:
    std::unique_ptr<EnhancedETIProcessorQt> m_etiProcessor;
    std::unique_ptr<AdvancedFIGAnalyzer> m_figAnalyzer;
    
    QLabel* m_processingLabel;
    QProgressBar* m_progressBar;
    QLabel* m_frameCountLabel;
    QLabel* m_figResultsLabel;
    QLabel* m_serviceResultsLabel;
    
    int m_frameCount = 0;
    int m_totalFrames = 0;
    QList<ProcessedFrame> m_processedFrames;
    QList<FIGData> m_processedFIGs;
    QList<ServiceInfo> m_discoveredServices;
};

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    
    Phase14TestWindow window;
    window.show();
    
    return app.exec();
}

#include "phase1_4_test.moc"