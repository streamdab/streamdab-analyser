#include <QApplication>
#include <QMainWindow>
#include <QVBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QFileDialog>
#include <QMessageBox>
#include <QTimer>
#include <QStatusBar>

#include "src/core/enhanced_eti_processor_qt.h"
#include "src/core/advanced_fig_analyzer.h"

class TestMainWindow : public QMainWindow {
    Q_OBJECT

public:
    TestMainWindow(QWidget *parent = nullptr) : QMainWindow(parent) {
        setWindowTitle("Phase 1.4 Qt MOC Test - Bangkok ETI Processing");
        setMinimumSize(800, 600);
        
        setupUI();
        setupProcessor();
    }

private slots:
    void openETIFile() {
        QString fileName = QFileDialog::getOpenFileName(
            this, "Open ETI File", "", "ETI Files (*.eti);;All Files (*)"
        );
        
        if (!fileName.isEmpty()) {
            statusBar()->showMessage(QString("Processing: %1").arg(fileName));
            
            // Test Phase 1.4 ETI processing
            if (processor_->processETIFile(fileName)) {
                QMessageBox::information(this, "Success", 
                    QString("Phase 1.4 ETI Processing Complete!\n\n"
                           "File: %1\n"
                           "Frames processed: %2\n"
                           "ETI-NI: %3\n"
                           "ETI-LI-A: %4\n"
                           "ETI-LI-B: %5\n\n"
                           "Qt MOC compilation successful!")
                           .arg(fileName)
                           .arg(processor_->getFrameCount())
                           .arg(processor_->getETINICount())
                           .arg(processor_->getETILIACount())
                           .arg(processor_->getETILIBCount()));
            } else {
                QMessageBox::warning(this, "Error", "Failed to process ETI file");
            }
        }
    }
    
    void onFrameProcessed(const ProcessedFrame& frame) {
        frameLabel_->setText(QString("Frame %1: %2 (%3)")
                            .arg(frame.frame_number)
                            .arg(processor_->formatToString(frame.format))
                            .arg(processor_->syncPatternToString(frame.sync_pattern)));
    }
    
    void onProcessingComplete(int totalFrames, int processingTimeMs) {
        statusBar()->showMessage(QString("Complete: %1 frames in %2ms")
                               .arg(totalFrames).arg(processingTimeMs));
    }

private:
    void setupUI() {
        auto *centralWidget = new QWidget(this);
        setCentralWidget(centralWidget);
        
        auto *layout = new QVBoxLayout(centralWidget);
        
        auto *titleLabel = new QLabel("Phase 1.4 Qt MOC Test - Bangkok ETI Analysis", this);
        titleLabel->setAlignment(Qt::AlignCenter);
        titleLabel->setStyleSheet("font-size: 16px; font-weight: bold; margin: 10px;");
        layout->addWidget(titleLabel);
        
        frameLabel_ = new QLabel("No frame processed yet", this);
        frameLabel_->setAlignment(Qt::AlignCenter);
        layout->addWidget(frameLabel_);
        
        auto *openButton = new QPushButton("Open Bangkok ETI File", this);
        connect(openButton, &QPushButton::clicked, this, &TestMainWindow::openETIFile);
        layout->addWidget(openButton);
        
        statusBar()->showMessage("Ready - Phase 1.4 Qt MOC compilation successful");
    }
    
    void setupProcessor() {
        processor_ = new EnhancedETIProcessorQt(this);
        analyzer_ = new AdvancedFIGAnalyzer(this);
        
        // Connect Qt signals/slots (this tests MOC compilation)
        connect(processor_, &EnhancedETIProcessorQt::frameProcessed,
                this, &TestMainWindow::onFrameProcessed);
        connect(processor_, &EnhancedETIProcessorQt::processingComplete,
                this, &TestMainWindow::onProcessingComplete);
        
        // Test signal/slot with FIG analyzer (simplified for MOC test)
        // connect(processor_, &EnhancedETIProcessorQt::figDiscovered,
        //         analyzer_, &AdvancedFIGAnalyzer::analyzeFICData);
    }
    
    EnhancedETIProcessorQt *processor_;
    AdvancedFIGAnalyzer *analyzer_;
    QLabel *frameLabel_;
};

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    
    TestMainWindow window;
    window.show();
    
    return app.exec();
}

#include "phase1_4_test_executable.moc"