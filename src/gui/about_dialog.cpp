/**
 * @file about_dialog.cpp
 * @brief Professional about dialog implementation for ETI Stream Analyzer
 * 
 * This file implements the AboutDialog class providing comprehensive
 * application information, credits, license, and system details for
 * professional broadcast environments.
 */

#include "about_dialog.h"
#include "utils/logger.h"

#include <QApplication>
#include <QClipboard>
#include <QMessageBox>
#include <QDateTime>
#include <QSysInfo>
#include <QThread>
#include <QScreen>
#include <QDesktopServices>
#include <QUrl>
#include <QDebug>

AboutDialog::AboutDialog(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::AboutDialog)
{
    // Initialize UI from Designer file
    ui->setupUi(this);
    
    // Center dialog on parent
    if (parent) {
        QRect parentGeometry = parent->geometry();
        int x = parentGeometry.x() + (parentGeometry.width() - width()) / 2;
        int y = parentGeometry.y() + (parentGeometry.height() - height()) / 2;
        move(x, y);
    }
    
    Logger::instance().log(Logger::Info, "AboutDialog", "Constructor completed");
}

AboutDialog::~AboutDialog()
{
    Logger::instance().log(Logger::Info, "AboutDialog", "Destructor starting");
    
    // Clean up UI
    delete ui;
    
    Logger::instance().log(Logger::Info, "AboutDialog", "Destructor completed");
}

bool AboutDialog::initialize()
{
    Logger::instance().log(Logger::Info, "AboutDialog", "Initialization starting");
    
    try {
        // Setup custom connections and populate content
        setupConnections();
        populateContent();
        
        Logger::instance().log(Logger::Info, "AboutDialog", "Initialization completed successfully");
        return true;
        
    } catch (const std::exception& e) {
        Logger::instance().log(Logger::Error, "AboutDialog", 
                             QString("Initialization failed: %1").arg(e.what()));
        return false;
    }
}

void AboutDialog::setupConnections()
{
    // Connect button box signals
    connect(ui->buttonBox, &QDialogButtonBox::rejected, this, &AboutDialog::handleClose);
    
    // Add copy info button to existing button box
    QPushButton *copyInfoButton = ui->buttonBox->addButton(tr("Copy Info"), QDialogButtonBox::ActionRole);
    connect(copyInfoButton, &QPushButton::clicked, this, &AboutDialog::handleCopyInfo);
    
    Logger::instance().log(Logger::Debug, "AboutDialog", "Signal connections completed");
}

void AboutDialog::populateContent()
{
    // Populate about tab with application logo
    QPixmap logo = getApplicationLogo();
    if (!logo.isNull()) {
        ui->logoLabel->setPixmap(logo.scaled(64, 64, Qt::KeepAspectRatio, Qt::SmoothTransformation));
        ui->logoLabel->setStyleSheet(""); // Remove placeholder style
    }
    
    // Populate credits tab with detailed credits text
    ui->creditsText->setPlainText(getCreditsText());
    
    // Populate license tab with license text
    ui->licenseText->setPlainText(getLicenseText());
    ui->licenseText->setFont(QFont("Courier", 8));
    
    // Populate system info tab with detailed system information
    ui->systemInfoText->setPlainText(getDetailedSystemInfo());
    ui->systemInfoText->setFont(QFont("Courier", 8));
    
    Logger::instance().log(Logger::Debug, "AboutDialog", "Content population completed");
}



QString AboutDialog::getVersionString()
{
    return QString("%1 %2").arg(APP_VERSION, APP_BUILD);
}

QString AboutDialog::getBuildInfo()
{
    return QString("Built on %1 %2 with Qt %3")
           .arg(__DATE__, __TIME__, QT_VERSION_STR);
}

QString AboutDialog::getSystemInfo()
{
    return QString("%1 %2 (%3)")
           .arg(QSysInfo::productType())
           .arg(QSysInfo::productVersion())
           .arg(QSysInfo::currentCpuArchitecture());
}

QPixmap AboutDialog::getApplicationLogo() const
{
    // Try to load application logo from resources
    QPixmap logo(":/icons/application.png");
    if (logo.isNull()) {
        // Create a simple placeholder logo
        logo = QPixmap(64, 64);
        logo.fill(Qt::lightGray);
    }
    return logo;
}

QString AboutDialog::getCreditsText() const
{
    return tr(
        "ETI Stream Analyzer Development Team\n"
        "====================================\n\n"
        
        "Core Development:\n"
        "• Lead Developer - ETI Processing Engine\n"
        "• GUI Developer - Qt Interface Design\n"
        "• Standards Compliance - ETSI Integration\n"
        "• Testing Engineer - Quality Assurance\n"
        "• Build Manager - Cross-platform Support\n"
        "• Network Engineer - Streaming Implementation\n\n"
        
        "Special Thanks:\n"
        "• ETISnoop Project - Original ETI processing foundation\n"
        "• DAB Industry Partners - Technical guidance and testing\n"
        "• Open Source Community - Libraries and tools\n"
        "• Professional Broadcast Engineers - Feedback and requirements\n\n"
        
        "Third-Party Libraries:\n"
        "• Qt Framework - Cross-platform GUI toolkit\n"
        "• libfaad2 - AAC audio decoder\n"
        "• libfftw3 - Fast Fourier Transform library\n"
        "• yaml-cpp - YAML configuration parser\n"
        "• Google Test - Testing framework\n"
        "• CMake - Build system\n\n"
        
        "Documentation and Resources:\n"
        "• ETSI EN 300 401 - DAB Standard\n"
        "• ETSI TS 102 563 - DAB+ Standard\n"
        "• Professional Broadcasting Community\n"
        "• Technical Standards Organizations\n\n"
        
        "Beta Testers:\n"
        "• Professional broadcast engineers\n"
        "• DAB network operators\n"
        "• Academic research institutions\n"
        "• Open source community contributors\n\n"
        
        "This software is developed with passion for professional\n"
        "broadcast monitoring and analysis. Thank you to everyone\n"
        "who contributed to making this project possible!"
    );
}

QString AboutDialog::getLicenseText() const
{
    return tr(
        "ETI Stream Analyzer License Agreement\n"
        "=====================================\n\n"
        
        "This software is released under the GNU General Public License v3.0\n\n"
        
        "Copyright (C) 2024 ETI Stream Analyzer Team\n\n"
        
        "This program is free software: you can redistribute it and/or modify\n"
        "it under the terms of the GNU General Public License as published by\n"
        "the Free Software Foundation, either version 3 of the License, or\n"
        "(at your option) any later version.\n\n"
        
        "This program is distributed in the hope that it will be useful,\n"
        "but WITHOUT ANY WARRANTY; without even the implied warranty of\n"
        "MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the\n"
        "GNU General Public License for more details.\n\n"
        
        "You should have received a copy of the GNU General Public License\n"
        "along with this program. If not, see <https://www.gnu.org/licenses/>.\n\n"
        
        "ADDITIONAL TERMS FOR PROFESSIONAL USE:\n"
        "======================================\n\n"
        
        "This software is designed for professional broadcast monitoring\n"
        "and analysis. While provided under GPL v3.0, users are encouraged\n"
        "to contribute improvements back to the community.\n\n"
        
        "For commercial licensing options or professional support,\n"
        "please contact the development team.\n\n"
        
        "THIRD-PARTY LICENSES:\n"
        "====================\n\n"
        
        "This software incorporates code from several open source projects:\n\n"
        
        "• Qt Framework - LGPL v3 / Commercial License\n"
        "• libfaad2 - GPL v2\n"
        "• libfftw3 - GPL v2 or later\n"
        "• yaml-cpp - MIT License\n"
        "• Google Test - BSD 3-Clause License\n\n"
        
        "Please refer to the individual license files for complete terms\n"
        "and conditions of these third-party components.\n\n"
        
        "DISCLAIMER:\n"
        "===========\n\n"
        
        "This software is provided 'as is' without warranty of any kind.\n"
        "The authors and contributors are not liable for any damages\n"
        "arising from the use of this software.\n\n"
        
        "Users are responsible for ensuring compliance with applicable\n"
        "broadcasting standards and regulations in their jurisdiction."
    );
}

QString AboutDialog::getDetailedSystemInfo() const
{
    QString info;
    
    // Application information
    info += QString("Application Information:\n");
    info += QString("========================\n");
    info += QString("Name: %1\n").arg(APP_NAME);
    info += QString("Version: %1\n").arg(getVersionString());
    info += QString("Build: %1\n").arg(getBuildInfo());
    info += QString("Executable: %1\n").arg(QApplication::applicationFilePath());
    info += QString("Working Directory: %1\n").arg(QApplication::applicationDirPath());
    info += QString("\n");
    
    // System information
    info += QString("System Information:\n");
    info += QString("===================\n");
    info += QString("Operating System: %1\n").arg(QSysInfo::prettyProductName());
    info += QString("Kernel Type: %1\n").arg(QSysInfo::kernelType());
    info += QString("Kernel Version: %1\n").arg(QSysInfo::kernelVersion());
    info += QString("Architecture: %1\n").arg(QSysInfo::currentCpuArchitecture());
    info += QString("Machine ID: %1\n").arg(QSysInfo::machineUniqueId().toHex());
    info += QString("\n");
    
    // Qt information
    info += QString("Qt Framework Information:\n");
    info += QString("========================\n");
    info += QString("Qt Version (Runtime): %1\n").arg(qVersion());
    info += QString("Qt Version (Compile): %1\n").arg(QT_VERSION_STR);
    info += QString("\n");
    
    // Hardware information
    info += QString("Hardware Information:\n");
    info += QString("====================\n");
    info += QString("CPU Count: %1\n").arg(QThread::idealThreadCount());
    
    // Screen information
    QScreen *screen = QApplication::primaryScreen();
    if (screen) {
        info += QString("Primary Screen: %1x%2 pixels\n")
                .arg(screen->size().width())
                .arg(screen->size().height());
        info += QString("Screen DPI: %1 x %2\n")
                .arg(screen->logicalDotsPerInchX())
                .arg(screen->logicalDotsPerInchY());
        info += QString("Device Pixel Ratio: %1\n").arg(screen->devicePixelRatio());
    }
    info += QString("\n");
    
    // Memory information (simplified)
    info += QString("Performance Information:\n");
    info += QString("=======================\n");
    info += QString("Available Processors: %1\n").arg(QThread::idealThreadCount());
    info += QString("Application PID: %1\n").arg(QApplication::applicationPid());
    info += QString("\n");
    
    // Environment
    info += QString("Environment:\n");
    info += QString("============\n");
    info += QString("PATH: %1\n").arg(qgetenv("PATH"));
    info += QString("HOME: %1\n").arg(qgetenv("HOME"));
    info += QString("USER: %1\n").arg(qgetenv("USER"));
    info += QString("\n");
    
    // Timestamp
    info += QString("Report Generated:\n");
    info += QString("================\n");
    info += QString("Date/Time: %1\n").arg(QDateTime::currentDateTime().toString(Qt::ISODate));
    info += QString("UTC Offset: %1\n").arg(QDateTime::currentDateTime().offsetFromUtc());
    
    return info;
}

// Private slots

void AboutDialog::handleClose()
{
    close();
    Logger::instance().log(Logger::Debug, "AboutDialog", "Dialog closed");
}

void AboutDialog::handleCopyInfo()
{
    QString info;
    info += QString("%1 %2\n").arg(APP_NAME, getVersionString());
    info += QString("%1\n").arg(getBuildInfo());
    info += QString("System: %1\n").arg(getSystemInfo());
    info += QString("Qt Version: %1\n").arg(qVersion());
    info += QString("Generated: %1\n").arg(QDateTime::currentDateTime().toString());
    
    QClipboard *clipboard = QApplication::clipboard();
    clipboard->setText(info);
    
    QMessageBox::information(this, tr("Information Copied"), 
                           tr("System information has been copied to the clipboard."));
    
    Logger::instance().log(Logger::Info, "AboutDialog", "System information copied to clipboard");
}