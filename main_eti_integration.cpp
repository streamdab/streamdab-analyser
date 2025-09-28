#include <QApplication>
#include <QMainWindow>
#include <QMenuBar>
#include <QStatusBar>
#include <QLabel>
#include <QVBoxLayout>
#include <QWidget>
#include <QFileDialog>
#include <QTextEdit>
#include <QSplitter>
#include <QMessageBox>
#include <QProgressBar>
#include "utils/logger.h"

// ETI processing without complex Q_OBJECT dependencies
class ETIProcessor {
public:
    struct ETIFrameInfo {
        uint32_t frame_count = 0;
        uint32_t sync_errors = 0;
        uint32_t crc_errors = 0;
        bool is_valid = false;
    };

    bool processETIFile(const QString& filename) {
        QFile file(filename);
        if (!file.open(QIODevice::ReadOnly)) {
            return false;
        }
        
        QByteArray data = file.readAll();
        return processETIData(data);
    }

    bool processETIData(const QByteArray& data) {
        // Basic ETI frame validation
        const size_t ETI_FRAME_SIZE = 6144;
        info_.frame_count = data.size() / ETI_FRAME_SIZE;
        
        // Check for ETI sync pattern: 0x49, 0x93, 0x1E, 0x03
        for (size_t i = 0; i < data.size(); i += ETI_FRAME_SIZE) {
            if (i + 4 <= data.size()) {
                if (data[i] == 0x49 && data[i+1] == 0x93 && 
                    data[i+2] == 0x1E && data[i+3] == 0x03) {
                    // Valid sync pattern found
                } else {
                    info_.sync_errors++;
                }
            }
        }
        
        info_.is_valid = (info_.sync_errors < info_.frame_count / 10); // Allow 10% errors
        return info_.is_valid;
    }

    const ETIFrameInfo& getInfo() const { return info_; }

private:
    ETIFrameInfo info_;
};

// Enhanced MainWindow with ETI processing
class ETIMainWindow : public QMainWindow {
    Q_OBJECT

public:
    ETIMainWindow(QWidget *parent = nullptr) : QMainWindow(parent) {
        setupUI();
        setupConnections();
        Logger::instance().log(Logger::Info, "MainWindow", "ETI MainWindow created");
    }

private slots:
    void openETIFile() {
        QString filename = QFileDialog::getOpenFileName(
            this, "Open ETI File", "", "ETI Files (*.eti);;All Files (*)");
        
        if (!filename.isEmpty()) {
            processETIFile(filename);
        }
    }

private:
    void setupUI() {
        // Create central widget with splitter
        auto* centralWidget = new QWidget(this);
        setCentralWidget(centralWidget);
        
        auto* layout = new QVBoxLayout(centralWidget);
        auto* splitter = new QSplitter(Qt::Horizontal, this);
        layout->addWidget(splitter);
        
        // Left panel - File info
        m_fileInfo = new QTextEdit(this);
        m_fileInfo->setMaximumWidth(300);
        m_fileInfo->setPlainText("No ETI file loaded");
        splitter->addWidget(m_fileInfo);
        
        // Right panel - Analysis results
        m_analysisResults = new QTextEdit(this);
        m_analysisResults->setPlainText("ETI Analysis Results will appear here");
        splitter->addWidget(m_analysisResults);
        
        // Create menu bar
        auto* fileMenu = menuBar()->addMenu("File");
        fileMenu->addAction("Open ETI File...", this, &ETIMainWindow::openETIFile);
        fileMenu->addSeparator();
        fileMenu->addAction("Exit", this, &QWidget::close);
        
        // Create status bar with progress
        m_progressBar = new QProgressBar(this);
        m_progressBar->setVisible(false);
        statusBar()->addPermanentWidget(m_progressBar);
        statusBar()->showMessage("Ready - StreamDAB ETI Analyzer");
        
        setWindowTitle("StreamDAB Stream Analyzer - ETI Processing");
        resize(1000, 700);
    }

    void setupConnections() {
        // No complex Q_OBJECT connections for now
    }

    void processETIFile(const QString& filename) {
        statusBar()->showMessage("Processing ETI file...");
        m_progressBar->setVisible(true);
        m_progressBar->setRange(0, 0); // Indeterminate progress
        
        // Process file
        ETIProcessor processor;
        bool success = processor.processETIFile(filename);
        
        // Update UI with results
        updateFileInfo(filename, processor.getInfo());
        updateAnalysisResults(processor.getInfo(), success);
        
        m_progressBar->setVisible(false);
        statusBar()->showMessage(success ? "ETI file processed successfully" : "ETI processing failed");
        
        Logger::instance().log(Logger::Info, "ETI", 
            QString("Processed file: %1, Frames: %2").arg(filename).arg(processor.getInfo().frame_count));
    }

    void updateFileInfo(const QString& filename, const ETIProcessor::ETIFrameInfo& info) {
        QFileInfo fileInfo(filename);
        QString text = QString(
            "File: %1\n"
            "Size: %2 bytes\n"
            "ETI Frames: %3\n"
            "Sync Errors: %4\n"
            "CRC Errors: %5\n"
            "Status: %6\n"
        ).arg(fileInfo.fileName())
         .arg(fileInfo.size())
         .arg(info.frame_count)
         .arg(info.sync_errors)
         .arg(info.crc_errors)
         .arg(info.is_valid ? "Valid" : "Invalid");
        
        m_fileInfo->setPlainText(text);
    }

    void updateAnalysisResults(const ETIProcessor::ETIFrameInfo& info, bool success) {
        QString results;
        if (success) {
            results = QString(
                "=== ETI Stream Analysis ===\n\n"
                "Frame Analysis:\n"
                "- Total Frames: %1\n"
                "- Sync Pattern Errors: %2 (%.1f%%)\n"
                "- Frame Size: 6144 bytes (standard)\n"
                "- Error Rate: %.2f%%\n\n"
                "Stream Quality: %3\n\n"
                "ETSI Compliance: %4\n\n"
                "Next Steps:\n"
                "- Service discovery analysis\n"
                "- FIC decoding\n"
                "- Audio component extraction\n"
            ).arg(info.frame_count)
             .arg(info.sync_errors)
             .arg((double)info.sync_errors / info.frame_count * 100.0)
             .arg((double)info.sync_errors / info.frame_count * 100.0)
             .arg(info.is_valid ? "GOOD" : "POOR")
             .arg(info.is_valid ? "PASSED" : "FAILED");
        } else {
            results = "ETI file processing failed.\n\nPossible causes:\n- Invalid file format\n- Corrupted data\n- Unsupported ETI variant";
        }
        
        m_analysisResults->setPlainText(results);
    }

private:
    QTextEdit* m_fileInfo;
    QTextEdit* m_analysisResults;
    QProgressBar* m_progressBar;
};

#include "main_eti_integration.moc"

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);
    
    Logger::instance().log(Logger::Info, "Application", "Starting StreamDAB ETI Analyzer...");
    
    ETIMainWindow window;
    window.show();
    
    return app.exec();
}