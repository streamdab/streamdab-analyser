/**
 * @file main_window_minimal.cpp
 * @brief Minimal MainWindow for Phase 1.4 Qt MOC testing
 */

#include "main_window.h"
#include <QVBoxLayout>
#include <QLabel>
#include <QMenuBar>
#include <QStatusBar>
#include <QFileDialog>
#include <QMessageBox>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    setWindowTitle("StreamDAB Analyzer - Phase 1.4");
    setMinimumSize(800, 600);
    
    // Create minimal central widget
    auto *centralWidget = new QWidget(this);
    setCentralWidget(centralWidget);
    
    auto *layout = new QVBoxLayout(centralWidget);
    auto *label = new QLabel("StreamDAB Phase 1.4 - Qt MOC Testing Ready", this);
    label->setAlignment(Qt::AlignCenter);
    layout->addWidget(label);
    
    // Create minimal menu
    auto *fileMenu = menuBar()->addMenu("&File");
    auto *openAction = fileMenu->addAction("&Open ETI File...");
    connect(openAction, &QAction::triggered, this, &MainWindow::openFile);
    
    statusBar()->showMessage("Ready - Phase 1.4 Build");
}

MainWindow::~MainWindow() = default;

void MainWindow::openFile()
{
    QString fileName = QFileDialog::getOpenFileName(
        this,
        "Open ETI File",
        "",
        "ETI Files (*.eti);;All Files (*)"
    );
    
    if (!fileName.isEmpty()) {
        statusBar()->showMessage(QString("Selected: %1").arg(fileName));
        QMessageBox::information(this, "Phase 1.4", 
            QString("File selected: %1\n\nQt MOC compilation successful!").arg(fileName));
    }
}