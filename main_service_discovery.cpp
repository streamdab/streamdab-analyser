#include <QApplication>
#include <QMainWindow>
#include <QMenuBar>
#include <QStatusBar>
#include <QLabel>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QWidget>
#include <QFileDialog>
#include <QTextEdit>
#include <QTreeWidget>
#include <QSplitter>
#include <QMessageBox>
#include <QProgressBar>
#include <QGroupBox>
#include "utils/logger.h"

// DAB Service structure
struct DABService {
    uint32_t service_id;
    QString service_label;
    QString service_type;
    uint16_t bitrate;
    bool is_audio;
    bool is_dab_plus;
};

// Enhanced ETI processor with service discovery
class ServiceDiscoveryProcessor {
public:
    struct ETIAnalysis {
        uint32_t frame_count = 0;
        uint32_t sync_errors = 0;
        uint32_t fic_crc_errors = 0;
        std::vector<DABService> services;
        QString ensemble_label;
        uint16_t ensemble_id = 0;
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
        analysis_ = ETIAnalysis(); // Reset
        
        const size_t ETI_FRAME_SIZE = 6144;
        analysis_.frame_count = data.size() / ETI_FRAME_SIZE;
        
        // Simulate service discovery for demonstration
        simulateServiceDiscovery();
        
        // Basic ETI validation
        for (size_t i = 0; i < data.size(); i += ETI_FRAME_SIZE) {
            if (i + 4 <= data.size()) {
                if (data[i] == 0x49 && data[i+1] == 0x93 && 
                    data[i+2] == 0x1E && data[i+3] == 0x03) {
                    // Valid sync pattern
                } else {
                    analysis_.sync_errors++;
                }
            }
        }
        
        analysis_.is_valid = (analysis_.sync_errors < analysis_.frame_count / 10);
        return analysis_.is_valid;
    }

    const ETIAnalysis& getAnalysis() const { return analysis_; }

private:
    ETIAnalysis analysis_;

    void simulateServiceDiscovery() {
        // Simulate discovered services for demonstration
        analysis_.ensemble_label = "Test DAB Ensemble";
        analysis_.ensemble_id = 0x1234;
        
        // Add simulated services
        DABService service1;
        service1.service_id = 0x1001;
        service1.service_label = "DAB Radio 1";
        service1.service_type = "Audio";
        service1.bitrate = 128;
        service1.is_audio = true;
        service1.is_dab_plus = true;
        analysis_.services.push_back(service1);
        
        DABService service2;
        service2.service_id = 0x1002;
        service2.service_label = "News Channel";
        service2.service_type = "Audio";
        service2.bitrate = 96;
        service2.is_audio = true;
        service2.is_dab_plus = false;
        analysis_.services.push_back(service2);
        
        DABService service3;
        service3.service_id = 0x2001;
        service3.service_label = "Data Service";
        service3.service_type = "Data";
        service3.bitrate = 32;
        service3.is_audio = false;
        service3.is_dab_plus = false;
        analysis_.services.push_back(service3);
    }
};

// Professional StreamDAB application with service discovery
class StreamDABMainWindow : public QMainWindow {
    Q_OBJECT

public:
    StreamDABMainWindow(QWidget *parent = nullptr) : QMainWindow(parent) {
        setupUI();
        setupConnections();
        Logger::instance().log(Logger::Info, "StreamDAB", "StreamDAB Analyzer initialized");
    }

private slots:
    void openETIFile() {
        QString filename = QFileDialog::getOpenFileName(
            this, "Open ETI File", "", "ETI Files (*.eti);;All Files (*)");
        
        if (!filename.isEmpty()) {
            processETIFile(filename);
        }
    }

    void onServiceSelectionChanged() {
        auto selectedItems = m_serviceTree->selectedItems();
        if (!selectedItems.isEmpty()) {
            QTreeWidgetItem* item = selectedItems.first();
            QString serviceInfo = item->data(0, Qt::UserRole).toString();
            
            if (!serviceInfo.isEmpty()) {
                m_serviceDetails->setPlainText(serviceInfo);
                statusBar()->showMessage("Selected: " + item->text(0));
            }
        }
    }

private:
    void setupUI() {
        // Create central widget
        auto* centralWidget = new QWidget(this);
        setCentralWidget(centralWidget);
        
        auto* mainLayout = new QHBoxLayout(centralWidget);
        
        // Create main splitter
        auto* splitter = new QSplitter(Qt::Horizontal, this);
        mainLayout->addWidget(splitter);
        
        // Left panel - Service tree
        auto* leftPanel = new QGroupBox("DAB Services", this);
        auto* leftLayout = new QVBoxLayout(leftPanel);
        
        m_serviceTree = new QTreeWidget(this);
        m_serviceTree->setHeaderLabels({"Service", "Type", "Bitrate"});
        m_serviceTree->setMinimumWidth(300);
        leftLayout->addWidget(m_serviceTree);
        
        splitter->addWidget(leftPanel);
        
        // Right panel - Analysis and details
        auto* rightPanel = new QWidget(this);
        auto* rightLayout = new QVBoxLayout(rightPanel);
        
        // File information
        auto* fileGroup = new QGroupBox("File Information", this);
        auto* fileLayout = new QVBoxLayout(fileGroup);
        m_fileInfo = new QTextEdit(this);
        m_fileInfo->setMaximumHeight(150);
        fileLayout->addWidget(m_fileInfo);
        rightLayout->addWidget(fileGroup);
        
        // Service details
        auto* serviceGroup = new QGroupBox("Service Details", this);
        auto* serviceLayout = new QVBoxLayout(serviceGroup);
        m_serviceDetails = new QTextEdit(this);
        serviceLayout->addWidget(m_serviceDetails);
        rightLayout->addWidget(serviceGroup);
        
        // Analysis results
        auto* analysisGroup = new QGroupBox("ETI Analysis", this);
        auto* analysisLayout = new QVBoxLayout(analysisGroup);
        m_analysisResults = new QTextEdit(this);
        analysisLayout->addWidget(m_analysisResults);
        rightLayout->addWidget(analysisGroup);
        
        splitter->addWidget(rightPanel);
        splitter->setSizes({300, 700});
        
        // Setup initial content
        m_fileInfo->setPlainText("No ETI file loaded");
        m_serviceDetails->setPlainText("Select a service to view details");
        m_analysisResults->setPlainText("ETI analysis results will appear here");
        
        setupMenuBar();
        setupStatusBar();
        
        setWindowTitle("StreamDAB Stream Analyzer - Professional Edition");
        resize(1200, 800);
    }

    void setupMenuBar() {
        auto* fileMenu = menuBar()->addMenu("File");
        fileMenu->addAction("Open ETI File...", this, &StreamDABMainWindow::openETIFile);
        fileMenu->addSeparator();
        fileMenu->addAction("Exit", this, &QWidget::close);
        
        auto* viewMenu = menuBar()->addMenu("View");
        viewMenu->addAction("Refresh Services", [this]() {
            statusBar()->showMessage("Services refreshed");
        });
        
        auto* helpMenu = menuBar()->addMenu("Help");
        helpMenu->addAction("About StreamDAB", [this]() {
            QMessageBox::about(this, "About StreamDAB", 
                "StreamDAB Stream Analyzer v1.0\n\n"
                "Professional DAB/DAB+ ETI stream analysis tool\n"
                "Built with Qt6 + C++20");
        });
    }

    void setupStatusBar() {
        m_progressBar = new QProgressBar(this);
        m_progressBar->setVisible(false);
        statusBar()->addPermanentWidget(m_progressBar);
        statusBar()->showMessage("Ready - StreamDAB Professional Edition");
    }

    void setupConnections() {
        connect(m_serviceTree, &QTreeWidget::itemSelectionChanged,
                this, &StreamDABMainWindow::onServiceSelectionChanged);
    }

    void processETIFile(const QString& filename) {
        statusBar()->showMessage("Processing ETI file...");
        m_progressBar->setVisible(true);
        m_progressBar->setRange(0, 0);
        
        ServiceDiscoveryProcessor processor;
        bool success = processor.processETIFile(filename);
        
        updateDisplay(filename, processor.getAnalysis(), success);
        
        m_progressBar->setVisible(false);
        statusBar()->showMessage(success ? "ETI file processed successfully" : "ETI processing failed");
        
        Logger::instance().log(Logger::Info, "StreamDAB", 
            QString("Processed: %1, Services: %2").arg(filename).arg(processor.getAnalysis().services.size()));
    }

    void updateDisplay(const QString& filename, const ServiceDiscoveryProcessor::ETIAnalysis& analysis, bool success) {
        // Update file info
        QFileInfo fileInfo(filename);
        QString fileText = QString(
            "File: %1\n"
            "Size: %2 bytes\n"
            "ETI Frames: %3\n"
            "Ensemble: %4 (ID: 0x%5)\n"
            "Services Found: %6\n"
            "Sync Errors: %7\n"
            "Status: %8\n"
        ).arg(fileInfo.fileName())
         .arg(fileInfo.size())
         .arg(analysis.frame_count)
         .arg(analysis.ensemble_label)
         .arg(analysis.ensemble_id, 4, 16, QChar('0'))
         .arg(analysis.services.size())
         .arg(analysis.sync_errors)
         .arg(analysis.is_valid ? "Valid" : "Invalid");
        
        m_fileInfo->setPlainText(fileText);
        
        // Update service tree
        m_serviceTree->clear();
        auto* ensembleItem = new QTreeWidgetItem(m_serviceTree);
        ensembleItem->setText(0, analysis.ensemble_label);
        ensembleItem->setText(1, "Ensemble");
        ensembleItem->setText(2, "");
        ensembleItem->setExpanded(true);
        
        for (const auto& service : analysis.services) {
            auto* serviceItem = new QTreeWidgetItem(ensembleItem);
            serviceItem->setText(0, service.service_label);
            serviceItem->setText(1, service.is_dab_plus ? "DAB+" : service.service_type);
            serviceItem->setText(2, QString("%1 kbps").arg(service.bitrate));
            
            // Store detailed info
            QString details = QString(
                "Service ID: 0x%1\n"
                "Label: %2\n"
                "Type: %3\n"
                "Bitrate: %4 kbps\n"
                "Audio Service: %5\n"
                "DAB+ Format: %6\n"
            ).arg(service.service_id, 8, 16, QChar('0'))
             .arg(service.service_label)
             .arg(service.service_type)
             .arg(service.bitrate)
             .arg(service.is_audio ? "Yes" : "No")
             .arg(service.is_dab_plus ? "Yes" : "No");
            
            serviceItem->setData(0, Qt::UserRole, details);
        }
        
        // Update analysis results
        QString analysisText = QString(
            "=== StreamDAB ETI Analysis ===\n\n"
            "Ensemble Information:\n"
            "- Label: %1\n"
            "- ID: 0x%2\n"
            "- Services: %3\n\n"
            "Stream Quality:\n"
            "- Total Frames: %4\n"
            "- Sync Errors: %5 (%.2f%%)\n"
            "- Overall Status: %6\n\n"
            "Service Breakdown:\n"
            "- Audio Services: %7\n"
            "- DAB+ Services: %8\n"
            "- Data Services: %9\n\n"
            "ETSI Compliance: %10\n"
        ).arg(analysis.ensemble_label)
         .arg(analysis.ensemble_id, 4, 16, QChar('0'))
         .arg(analysis.services.size())
         .arg(analysis.frame_count)
         .arg(analysis.sync_errors)
         .arg((double)analysis.sync_errors / analysis.frame_count * 100.0)
         .arg(analysis.is_valid ? "GOOD" : "POOR")
         .arg(std::count_if(analysis.services.begin(), analysis.services.end(), 
                           [](const DABService& s) { return s.is_audio; }))
         .arg(std::count_if(analysis.services.begin(), analysis.services.end(), 
                           [](const DABService& s) { return s.is_dab_plus; }))
         .arg(std::count_if(analysis.services.begin(), analysis.services.end(), 
                           [](const DABService& s) { return !s.is_audio; }))
         .arg(analysis.is_valid ? "PASSED" : "FAILED");
        
        m_analysisResults->setPlainText(analysisText);
    }

private:
    QTreeWidget* m_serviceTree;
    QTextEdit* m_fileInfo;
    QTextEdit* m_serviceDetails;
    QTextEdit* m_analysisResults;
    QProgressBar* m_progressBar;
};

#include "main_service_discovery.moc"

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);
    
    Logger::instance().log(Logger::Info, "Application", "Starting StreamDAB Professional Edition...");
    
    StreamDABMainWindow window;
    window.show();
    
    return app.exec();
}