#pragma once

#include <QDialog>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QLabel>
#include <QPushButton>
#include <QTextEdit>
#include <QTabWidget>
#include <QGroupBox>
#include <QDialogButtonBox>
#include <QScrollArea>
#include <QPixmap>
#include <QIcon>
#include <QString>
#include <QStringList>

#include "core/product_version.hpp"

/**
 * @class AboutDialog
 * @brief Professional about dialog for StreamDAB Analyser
 * 
 * This dialog provides comprehensive information about the StreamDAB Analyser
 * application including version details, credits, license information, and
 * system information for professional broadcast environments.
 */
class AboutDialog : public QDialog
{
    Q_OBJECT

public:
    /**
     * @brief Constructor
     * @param parent Parent widget
     */
    explicit AboutDialog(QWidget *parent = nullptr);

    /**
     * @brief Destructor
     */
    ~AboutDialog();

    /**
     * @brief Initialize the about dialog
     * @return true if successful
     */
    bool initialize();

    /**
     * @brief Get application version string
     * @return Version string
     */
    static QString getVersionString();

    /**
     * @brief Get build information
     * @return Build info string
     */
    static QString getBuildInfo();

    /**
     * @brief Get system information
     * @return System info string
     */
    static QString getSystemInfo();

private slots:
    /**
     * @brief Handle close button click
     */
    void handleClose();

    /**
     * @brief Handle copy info button click
     */
    void handleCopyInfo();

private:
    /**
     * @brief Create the user interface
     */
    void createUI();

    /**
     * @brief Setup signal connections
     */
    void setupConnections();

    /**
     * @brief Populate content in UI elements
     */
    void populateContent();

    /**
     * @brief Get application logo
     * @return Application logo pixmap
     */
    QPixmap getApplicationLogo() const;

    /**
     * @brief Get credits information
     * @return Credits text
     */
    QString getCreditsText() const;

    /**
     * @brief Get license text
     * @return License text
     */
    QString getLicenseText() const;

    /**
     * @brief Get detailed system information
     * @return Detailed system info
     */
    QString getDetailedSystemInfo() const;

    // UI components
    QTabWidget *m_tabWidget;
    QDialogButtonBox *m_buttonBox;
    
    // About tab
    QWidget *m_aboutTab;
    QLabel *m_logoLabel;
    QLabel *m_titleLabel;
    QLabel *m_versionLabel;
    QLabel *m_copyrightLabel;
    QLabel *m_descriptionLabel;
    QPushButton *m_copyInfoButton;
    
    // Credits tab
    QWidget *m_creditsTab;
    QTextEdit *m_creditsText;
    
    // License tab
    QWidget *m_licenseTab;
    QTextEdit *m_licenseText;
    
    // System Info tab
    QWidget *m_systemTab;
    QTextEdit *m_systemInfoText;

    // Application information
    static constexpr const char* APP_NAME = "StreamDAB Analyser";
    static constexpr const char* APP_VERSION = DABX_VERSION;
    static constexpr const char* APP_BUILD = "Open Source Edition (GPL-3.0+)";
    static constexpr const char* APP_COPYRIGHT = "Copyright © 2026 StreamDAB Analyser contributors";
    static constexpr const char* APP_WEBSITE = "https://github.com/streamdab/streamdab-analyser";
};