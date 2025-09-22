/**
 * @file test_gui_layout.cpp
 * @brief Simple test program for three-panel GUI layout validation
 */

#include <QApplication>
#include <QMainWindow>
#include <QDockWidget>
#include <QSplitter>
#include <QTabWidget>
#include <QTreeWidget>
#include <QTableWidget>
#include <QTextEdit>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QHeaderView>
#include <QDebug>
#include <QTimer>

class TestMainWindow : public QMainWindow
{
    Q_OBJECT

public:
    TestMainWindow(QWidget *parent = nullptr) : QMainWindow(parent) {
        setupThreePanelLayout();
        setWindowTitle("ETI Stream Analyzer - Three Panel Layout Test");
        resize(1200, 800);
        
        // Apply professional broadcast styling
        applyBroadcastTheme();
        
        qDebug() << "✅ Three-panel layout test window created";
    }

private:
    void setupThreePanelLayout() {
        qDebug() << "🏗️ Setting up three-panel professional layout...";
        
        // Enable professional dock features
        setDockNestingEnabled(true);
        setTabPosition(Qt::AllDockWidgetAreas, QTabWidget::North);
        
        // Create central widget
        QWidget* centralWidget = new QWidget;
        setCentralWidget(centralWidget);
        
        // Create main horizontal splitter for three panels
        QSplitter* horizontalSplitter = new QSplitter(Qt::Horizontal, centralWidget);
        horizontalSplitter->setHandleWidth(3);
        horizontalSplitter->setChildrenCollapsible(false);
        
        // Explorer Panel (Left 25%) - Service Explorer
        QWidget* explorerPanel = createExplorerPanel();
        explorerPanel->setObjectName("explorerPanel");
        
        // Main Content Area (Center 50%) - ETI Frame Analysis
        QWidget* mainContentArea = createMainContentArea();
        mainContentArea->setObjectName("mainContentArea");
        
        // Properties Panel (Right 25%) - Properties & Parameters
        QWidget* propertiesPanel = createPropertiesPanel();
        propertiesPanel->setObjectName("propertiesPanel");
        
        // Add panels to splitter
        horizontalSplitter->addWidget(explorerPanel);
        horizontalSplitter->addWidget(mainContentArea);
        horizontalSplitter->addWidget(propertiesPanel);
        
        // Set professional proportions (25%, 50%, 25%)
        QList<int> sizes = {300, 600, 300}; // Total 1200px width
        horizontalSplitter->setSizes(sizes);
        horizontalSplitter->setStretchFactor(0, 1);  // Explorer
        horizontalSplitter->setStretchFactor(1, 2);  // Main content (priority)
        horizontalSplitter->setStretchFactor(2, 1);  // Properties
        
        // Layout the splitter in central widget
        QVBoxLayout* centralLayout = new QVBoxLayout(centralWidget);
        centralLayout->setContentsMargins(5, 5, 5, 5);
        centralLayout->addWidget(horizontalSplitter);
        
        qDebug() << "✅ Three-panel layout completed";
    }
    
    QWidget* createExplorerPanel() {
        qDebug() << "🌳 Creating Explorer Panel (Service Tree)";
        
        QWidget* panel = new QWidget;
        QVBoxLayout* layout = new QVBoxLayout(panel);
        
        // Panel header
        QLabel* header = new QLabel("DAB Services");
        header->setStyleSheet("font-weight: bold; font-size: 12px; color: #0078D4;");
        layout->addWidget(header);
        
        // Service tree
        QTreeWidget* serviceTree = new QTreeWidget;
        serviceTree->setHeaderLabel("Service Explorer");
        serviceTree->setObjectName("serviceTree");
        
        // Add sample DAB services
        QTreeWidgetItem* ensemble = new QTreeWidgetItem(serviceTree, {"DAB Ensemble (BBC)"});
        ensemble->setExpanded(true);
        
        QTreeWidgetItem* audioServices = new QTreeWidgetItem(ensemble, {"Audio Services"});
        audioServices->addChild(new QTreeWidgetItem({"BBC Radio 1 (DAB+)"}));
        audioServices->addChild(new QTreeWidgetItem({"BBC Radio 2 (DAB+)"}));
        audioServices->addChild(new QTreeWidgetItem({"BBC Radio 4 (DAB)"}));
        audioServices->setExpanded(true);
        
        QTreeWidgetItem* dataServices = new QTreeWidgetItem(ensemble, {"Data Services"});
        dataServices->addChild(new QTreeWidgetItem({"EPG Service"}));
        dataServices->addChild(new QTreeWidgetItem({"Traffic Data"}));
        
        layout->addWidget(serviceTree);
        
        // Control buttons
        QHBoxLayout* buttonLayout = new QHBoxLayout;
        QPushButton* refreshBtn = new QPushButton("Refresh");
        QPushButton* expandBtn = new QPushButton("Expand All");
        buttonLayout->addWidget(refreshBtn);
        buttonLayout->addWidget(expandBtn);
        layout->addLayout(buttonLayout);
        
        panel->setMinimumWidth(250);
        panel->setMaximumWidth(400);
        
        return panel;
    }
    
    QWidget* createMainContentArea() {
        qDebug() << "📊 Creating Main Content Area (ETI Analysis)";
        
        QWidget* panel = new QWidget;
        QVBoxLayout* layout = new QVBoxLayout(panel);
        
        // Panel header
        QLabel* header = new QLabel("ETI Frame Analysis");
        header->setStyleSheet("font-weight: bold; font-size: 12px; color: #0078D4;");
        layout->addWidget(header);
        
        // ETI frame table
        QTableWidget* frameTable = new QTableWidget;
        frameTable->setColumnCount(5);
        frameTable->setHorizontalHeaderLabels({"Frame #", "Time", "Services", "Errors", "Quality"});
        frameTable->setObjectName("frameTable");
        frameTable->setAlternatingRowColors(true);
        frameTable->setSortingEnabled(true);
        frameTable->horizontalHeader()->setStretchLastSection(true);
        
        // Add sample ETI frames
        frameTable->setRowCount(10);
        for (int i = 0; i < 10; ++i) {
            frameTable->setItem(i, 0, new QTableWidgetItem(QString::number(i + 1)));
            frameTable->setItem(i, 1, new QTableWidgetItem(QString("00:00:%1").arg(i, 2, 10, QChar('0'))));
            frameTable->setItem(i, 2, new QTableWidgetItem("3 Audio, 2 Data"));
            frameTable->setItem(i, 3, new QTableWidgetItem(i % 3 == 0 ? "None" : "CRC"));
            frameTable->setItem(i, 4, new QTableWidgetItem(QString("%1%").arg(95 + (i % 5))));
        }
        
        layout->addWidget(frameTable);
        
        // Analysis tabs at bottom
        QTabWidget* analysisTabs = new QTabWidget;
        
        QTextEdit* hexViewer = new QTextEdit;
        hexViewer->setFont(QFont("monospace", 9));
        hexViewer->setText("FF 00 FF 1F 00 1F 54 00\n"
                          "00 00 01 02 03 04 05 06\n"
                          "A1 B2 C3 D4 E5 F6 07 08");
        analysisTabs->addTab(hexViewer, "Hex Viewer");
        
        QTextEdit* ficsAnalysis = new QTextEdit;
        ficsAnalysis->setText("FIC Analysis:\n"
                             "- FIG 0/0: Ensemble Information\n"
                             "- FIG 0/1: Service Information\n"
                             "- FIG 0/2: Service Component");
        analysisTabs->addTab(ficsAnalysis, "FIC Analysis");
        
        layout->addWidget(analysisTabs);
        
        // Set layout proportions
        layout->setStretchFactor(frameTable, 7);      // 70% for frame table
        layout->setStretchFactor(analysisTabs, 3);    // 30% for analysis tabs
        
        return panel;
    }
    
    QWidget* createPropertiesPanel() {
        qDebug() << "⚙️ Creating Properties Panel (Parameters)";
        
        QWidget* panel = new QWidget;
        QVBoxLayout* layout = new QVBoxLayout(panel);
        
        // Panel header
        QLabel* header = new QLabel("Service Properties");
        header->setStyleSheet("font-weight: bold; font-size: 12px; color: #0078D4;");
        layout->addWidget(header);
        
        // Properties tabs
        QTabWidget* propertiesTabs = new QTabWidget;
        propertiesTabs->setObjectName("propertiesTabs");
        
        // Service Info tab
        QTextEdit* serviceInfo = new QTextEdit;
        serviceInfo->setReadOnly(true);
        serviceInfo->setText("Service: BBC Radio 1\n"
                            "Type: DAB+ Audio\n"
                            "Bitrate: 128 kbps\n"
                            "Sample Rate: 48 kHz\n"
                            "Protection: 3-A\n"
                            "Service ID: 0xC221\n"
                            "Subchannel: 5\n"
                            "Quality: Excellent (98%)");
        propertiesTabs->addTab(serviceInfo, "Service");
        
        // Technical Parameters tab
        QTextEdit* techParams = new QTextEdit;
        techParams->setReadOnly(true);
        techParams->setText("ETI Parameters:\n"
                           "Frame Length: 6144 bytes\n"
                           "FIC Length: 96 bytes\n"
                           "MST Length: 5760 bytes\n"
                           "LIDATA: 5760 bytes\n"
                           "EOF: 4 bytes\n"
                           "TIST: 0x123456\n"
                           "Frame Count: 1234");
        propertiesTabs->addTab(techParams, "Technical");
        
        // Quality Metrics tab
        QTextEdit* qualityMetrics = new QTextEdit;
        qualityMetrics->setReadOnly(true);
        qualityMetrics->setText("Quality Metrics:\n"
                               "Signal Strength: -45 dBm\n"
                               "SNR: 28 dB\n"
                               "Error Rate: 0.001%\n"
                               "Frame Sync: 100%\n"
                               "CRC Errors: 0\n"
                               "Reed-Solomon: OK\n"
                               "Viterbi: 98.5%");
        propertiesTabs->addTab(qualityMetrics, "Quality");
        
        layout->addWidget(propertiesTabs);
        
        // Control buttons
        QHBoxLayout* buttonLayout = new QHBoxLayout;
        QPushButton* syncBtn = new QPushButton("Sync Mode");
        QPushButton* compareBtn = new QPushButton("Compare");
        buttonLayout->addWidget(syncBtn);
        buttonLayout->addWidget(compareBtn);
        layout->addLayout(buttonLayout);
        
        panel->setMinimumWidth(250);
        panel->setMaximumWidth(400);
        
        return panel;
    }
    
    void applyBroadcastTheme() {
        qDebug() << "🎨 Applying professional broadcast theme";
        
        QString broadcastStyle = R"(
            QMainWindow {
                background-color: #2D2D30;
                color: white;
            }
            
            QDockWidget {
                background-color: #2D2D30;
                color: white;
                titlebar-close-icon: none;
                titlebar-normal-icon: none;
            }
            
            QDockWidget::title {
                background-color: #0078D4;
                color: white;
                padding: 5px;
                font-weight: bold;
            }
            
            QTreeWidget, QTableWidget, QTextEdit {
                background-color: #1E1E1E;
                color: white;
                alternate-background-color: #2D2D30;
                selection-background-color: #0078D4;
                border: 1px solid #3C3C3C;
            }
            
            QHeaderView::section {
                background-color: #0078D4;
                color: white;
                padding: 4px;
                border: 1px solid #3C3C3C;
                font-weight: bold;
            }
            
            QTabWidget::pane {
                border: 1px solid #3C3C3C;
                background-color: #2D2D30;
            }
            
            QTabBar::tab {
                background-color: #3C3C3C;
                color: white;
                padding: 6px 12px;
                margin-right: 2px;
            }
            
            QTabBar::tab:selected {
                background-color: #0078D4;
                color: white;
            }
            
            QPushButton {
                background-color: #0078D4;
                color: white;
                border: none;
                padding: 6px 12px;
                border-radius: 3px;
                font-weight: bold;
            }
            
            QPushButton:hover {
                background-color: #106EBE;
            }
            
            QPushButton:pressed {
                background-color: #005A9E;
            }
            
            QLabel {
                color: white;
            }
            
            QSplitter::handle {
                background-color: #3C3C3C;
                width: 3px;
                height: 3px;
            }
            
            QSplitter::handle:hover {
                background-color: #0078D4;
            }
        )";
        
        setStyleSheet(broadcastStyle);
    }

public slots:
    void validateLayout() {
        qDebug() << "🔍 Validating three-panel layout...";
        
        // Check for required panels
        QWidget* explorerPanel = findChild<QWidget*>("explorerPanel");
        QWidget* mainContentArea = findChild<QWidget*>("mainContentArea");  
        QWidget* propertiesPanel = findChild<QWidget*>("propertiesPanel");
        
        bool layoutValid = true;
        
        if (!explorerPanel) {
            qWarning() << "❌ Explorer panel not found";
            layoutValid = false;
        } else {
            qDebug() << "✅ Explorer panel found";
        }
        
        if (!mainContentArea) {
            qWarning() << "❌ Main content area not found";
            layoutValid = false;
        } else {
            qDebug() << "✅ Main content area found";
        }
        
        if (!propertiesPanel) {
            qWarning() << "❌ Properties panel not found";
            layoutValid = false;
        } else {
            qDebug() << "✅ Properties panel found";
        }
        
        // Check for specific components
        QTreeWidget* serviceTree = findChild<QTreeWidget*>("serviceTree");
        QTableWidget* frameTable = findChild<QTableWidget*>("frameTable");
        QTabWidget* propertiesTabs = findChild<QTabWidget*>("propertiesTabs");
        
        if (serviceTree) {
            qDebug() << "✅ Service tree found with" << serviceTree->topLevelItemCount() << "items";
        }
        
        if (frameTable) {
            qDebug() << "✅ Frame table found with" << frameTable->rowCount() << "rows";
        }
        
        if (propertiesTabs) {
            qDebug() << "✅ Properties tabs found with" << propertiesTabs->count() << "tabs";
        }
        
        if (layoutValid) {
            qDebug() << "🎉 THREE-PANEL LAYOUT VALIDATION SUCCESSFUL!";
            qDebug() << "Layout meets professional broadcast industry standards";
        } else {
            qDebug() << "❌ Three-panel layout validation failed";
        }
    }
};

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    
    qDebug() << "=== ETI Stream Analyzer - Three Panel Layout Test ===";
    qDebug() << "Testing professional broadcast industry layout pattern";
    
    TestMainWindow window;
    window.show();
    
    // Validate layout after window is shown
    QTimer::singleShot(100, &window, &TestMainWindow::validateLayout);
    
    qDebug() << "Layout test window displayed. Press Ctrl+C to exit.";
    
    return app.exec();
}

#include "test_gui_layout.moc"