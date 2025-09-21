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
    , m_tabWidget(nullptr)
    , m_buttonBox(nullptr)
{
    // Create UI manually
    createUI();
    
    Logger::instance().log(Logger::Info, "AboutDialog", "Constructor completed");
}

AboutDialog::~AboutDialog()
{
    Logger::instance().log(Logger::Info, "AboutDialog", "Destructor starting");
    
    // Qt handles widget cleanup automatically
    
    Logger::instance().log(Logger::Info, "AboutDialog", "Destructor completed");
}

void AboutDialog::createUI()
{
    // Create main layout
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    
    // Create tab widget
    m_tabWidget = new QTabWidget(this);
    
    // Create About tab
    m_aboutTab = new QWidget();
    QVBoxLayout *aboutLayout = new QVBoxLayout(m_aboutTab);
    
    // Application info section
    QHBoxLayout *headerLayout = new QHBoxLayout();
    
    // Logo
    m_logoLabel = new QLabel();
    m_logoLabel->setPixmap(getApplicationLogo().scaled(64, 64, Qt::KeepAspectRatio, Qt::SmoothTransformation));
    m_logoLabel->setAlignment(Qt::AlignCenter);
    headerLayout->addWidget(m_logoLabel);
    
    // Title and version info
    QVBoxLayout *infoLayout = new QVBoxLayout();
    
    m_titleLabel = new QLabel(QString("<h2>%1</h2>").arg(APP_NAME));
    m_titleLabel->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    
    m_versionLabel = new QLabel(QString("<b>Version:</b> %1 (%2)").arg(APP_VERSION, APP_BUILD));
    
    m_copyrightLabel = new QLabel(APP_COPYRIGHT);
    
    m_descriptionLabel = new QLabel(
        tr("Professional ETI Stream Analyzer for broadcast DAB/DAB+ monitoring and analysis. "
           "Built on ETISnoop technology with advanced Qt GUI framework for comprehensive "
           "stream analysis and real-time monitoring capabilities.")
    );
    m_descriptionLabel->setWordWrap(true);
    
    infoLayout->addWidget(m_titleLabel);
    infoLayout->addWidget(m_versionLabel);
    infoLayout->addWidget(m_copyrightLabel);
    infoLayout->addWidget(m_descriptionLabel);
    infoLayout->addStretch();
    
    headerLayout->addLayout(infoLayout);
    
    // Copy info button
    m_copyInfoButton = new QPushButton(tr("Copy System Info"));
    connect(m_copyInfoButton, &QPushButton::clicked, this, &AboutDialog::handleCopyInfo);
    
    aboutLayout->addLayout(headerLayout);
    aboutLayout->addWidget(m_copyInfoButton);
    aboutLayout->addStretch();
    
    m_tabWidget->addTab(m_aboutTab, tr("About"));
    
    // Create Credits tab
    m_creditsTab = new QWidget();
    QVBoxLayout *creditsLayout = new QVBoxLayout(m_creditsTab);
    
    m_creditsText = new QTextEdit();
    m_creditsText->setReadOnly(true);
    m_creditsText->setPlainText(getCreditsText());
    
    creditsLayout->addWidget(m_creditsText);
    m_tabWidget->addTab(m_creditsTab, tr("Credits"));
    
    // Create License tab
    m_licenseTab = new QWidget();
    QVBoxLayout *licenseLayout = new QVBoxLayout(m_licenseTab);
    
    m_licenseText = new QTextEdit();
    m_licenseText->setReadOnly(true);
    m_licenseText->setPlainText(getLicenseText());
    
    licenseLayout->addWidget(m_licenseText);
    m_tabWidget->addTab(m_licenseTab, tr("License"));
    
    // Create System Info tab
    m_systemTab = new QWidget();
    QVBoxLayout *systemLayout = new QVBoxLayout(m_systemTab);
    
    m_systemInfoText = new QTextEdit();
    m_systemInfoText->setReadOnly(true);
    m_systemInfoText->setPlainText(getDetailedSystemInfo());
    
    systemLayout->addWidget(m_systemInfoText);
    m_tabWidget->addTab(m_systemTab, tr("System Info"));
    
    // Create button box
    m_buttonBox = new QDialogButtonBox(QDialogButtonBox::Close, this);
    connect(m_buttonBox, &QDialogButtonBox::accepted, this, &AboutDialog::accept);
    connect(m_buttonBox, &QDialogButtonBox::rejected, this, &AboutDialog::reject);
    
    // Add to main layout
    mainLayout->addWidget(m_tabWidget);
    mainLayout->addWidget(m_buttonBox);
    
    // Set dialog properties
    setObjectName("AboutDialog");
    setWindowTitle(tr("About %1").arg(APP_NAME));
    setWindowIcon(QIcon(":/icons/about.png"));
    setModal(true);
    setMinimumSize(500, 400);
    resize(600, 500);
    
    // Center dialog on parent
    if (parentWidget()) {
        QRect parentGeometry = parentWidget()->geometry();
        int x = parentGeometry.x() + (parentGeometry.width() - width()) / 2;
        int y = parentGeometry.y() + (parentGeometry.height() - height()) / 2;
        move(x, y);
    }
    
    // Setup connections and populate content
    setupConnections();
    populateContent();
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
    // Connections are already set up in createUI()
    Logger::instance().log(Logger::Debug, "AboutDialog", "Signal connections completed");
}

void AboutDialog::populateContent()
{
    // Populate about tab with application logo
    QPixmap logo = getApplicationLogo();
    if (!logo.isNull() && m_logoLabel) {
        m_logoLabel->setPixmap(logo.scaled(64, 64, Qt::KeepAspectRatio, Qt::SmoothTransformation));
        m_logoLabel->setStyleSheet(""); // Remove placeholder style
    }
    
    // Populate credits tab with detailed credits text
    if (m_creditsText) {
        m_creditsText->setPlainText(getCreditsText());
    }
    
    // Populate license tab with license text
    if (m_licenseText) {
        m_licenseText->setPlainText(getLicenseText());
        m_licenseText->setFont(QFont("Courier", 8));
    }
    
    // Populate system info tab with detailed system information
    if (m_systemInfoText) {
        m_systemInfoText->setPlainText(getDetailedSystemInfo());
        m_systemInfoText->setFont(QFont("Courier", 8));
    }
    
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