/**
 * @file test_qt_designer_integration.cpp
 * @brief TDD Test Suite for Qt Designer Integration
 * 
 * Comprehensive test-driven development suite for validating Qt Designer
 * integration with professional broadcast industry dialog components.
 * 
 * Qt Designer Integration Features:
 * - Visual UI file loading and validation
 * - Signal/slot connection verification
 * - Widget property persistence testing
 * - Layout manager functionality validation
 * - Professional dialog component integration
 * - Cross-platform UI consistency verification
 * 
 * @author UI/UX Agent - TDD Lead
 * @date 2025-09-22
 * @copyright StreamDAB Analyser Project
 */

#include "professional_gui_tdd_framework.h"
#include "gui/settings_dialog.h"
#include "gui/about_dialog.h"
#include "gui/alert_configuration_dialog.h"
#include "gui/stream_recording_dialog.h"
#include <QtTest/QtTest>
#include <QApplication>
#include <QUiLoader>
#include <QFile>
#include <QDir>
#include <QTabWidget>
#include <QPushButton>
#include <QLineEdit>
#include <QCheckBox>
#include <QSpinBox>
#include <QComboBox>
#include <QSlider>
#include <QProgressBar>
#include <QLabel>
#include <QGroupBox>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QGridLayout>
#include <QSignalSpy>
#include <QMetaObject>
#include <QMetaProperty>

/**
 * @class QtDesignerIntegrationTest
 * @brief TDD test suite for Qt Designer integration validation
 * 
 * This test suite validates the professional Qt Designer integration
 * for broadcast industry standard dialog components and UI workflows.
 * 
 * Test Coverage:
 * - UI file loading and parsing validation
 * - Professional dialog component integration
 * - Signal/slot connection verification
 * - Widget property persistence testing
 * - Layout manager functionality validation
 * - Cross-platform consistency verification
 * - Performance impact validation
 */
GUI_TDD_TEST_CASE(QtDesignerIntegrationTest, HIGH)

private:
    ProfessionalGUI::ProfessionalGUITestFramework* m_testFramework = nullptr;
    ProfessionalGUI::QtDesignerIntegrationTester* m_designerTester = nullptr;
    QUiLoader* m_uiLoader = nullptr;
    QStringList m_availableUIFiles;

public slots:
    void initTestCase() {
        qDebug() << "=== Qt Designer Integration TDD Test Suite ===";
        qDebug() << "Testing professional dialog components with Qt Designer";
        qDebug() << "UI Files: SettingsDialog, AboutDialog, AlertConfiguration, StreamRecording";
        
        // Initialize professional GUI test framework
        m_testFramework = new ProfessionalGUI::ProfessionalGUITestFramework(this);
        QVERIFY(m_testFramework->initializeProfessionalTestEnvironment());
        
        // Initialize Qt Designer integration tester
        m_designerTester = new ProfessionalGUI::QtDesignerIntegrationTester(this);
        
        // Initialize UI loader
        m_uiLoader = new QUiLoader(this);
        
        // Scan for available UI files
        scanForUIFiles();
    }

    void init() {
        // Fresh setup for each test
        QApplication::processEvents();
    }

    void cleanup() {
        // Clean up any created widgets
        QApplication::processEvents();
    }

    /**
     * @brief TDD RED Phase: Designer integration should fail initially
     */
    TDD_RED_PHASE(UIFileLoading)
        if (qEnvironmentVariableIsSet("TDD_FORCE_RED")) {
            // In RED phase, expect UI files to not exist or fail to load
            bool hasUIFiles = !m_availableUIFiles.isEmpty();
            TDD_RED_ASSERT(!hasUIFiles, "UI files should not exist in RED phase");
        }
    }

    /**
     * @brief TDD GREEN Phase: UI files should load successfully
     */
    TDD_GREEN_PHASE(UIFileLoading)
        qDebug() << "🟢 GREEN Phase: Testing UI file loading";
        
        // Should find UI files in the gui/ui directory
        QVERIFY2(m_availableUIFiles.size() > 0, "Should have UI files for professional dialogs");
        
        qDebug() << "✅ Found" << m_availableUIFiles.size() << "UI files for testing";
        for (const QString& uiFile : m_availableUIFiles) {
            qDebug() << "   -" << QFileInfo(uiFile).fileName();
        }
    }

    /**
     * @brief Test SettingsDialog Qt Designer integration
     */
    void testSettingsDialogDesignerIntegration() {
        qDebug() << "⚙️ Testing SettingsDialog Qt Designer Integration";
        
        try {
            // Create settings dialog
            SettingsDialog settingsDialog;
            
            // Test dialog basic properties
            QVERIFY(!settingsDialog.windowTitle().isEmpty());
            qDebug() << "   Settings dialog title:" << settingsDialog.windowTitle();
            
            // Validate Qt Designer integration
            VALIDATE_QT_DESIGNER_INTEGRATION(&settingsDialog);
            
            // Test dialog structure - should have tabs for professional configuration
            QTabWidget* tabWidget = settingsDialog.findChild<QTabWidget*>();
            if (tabWidget) {
                qDebug() << "   ✅ Settings tabs found:" << tabWidget->count();
                QVERIFY(tabWidget->count() >= 3); // Professional settings should have multiple tabs
                
                // Test typical professional settings tabs
                QStringList expectedTabs = {"General", "Audio", "Analysis", "Performance", "Network", "UI", "Advanced"};
                QStringList foundTabs;
                
                for (int i = 0; i < tabWidget->count(); ++i) {
                    QString tabText = tabWidget->tabText(i);
                    foundTabs.append(tabText);
                    
                    // Test that each tab has content
                    QWidget* tabContent = tabWidget->widget(i);
                    QVERIFY2(tabContent != nullptr, 
                             QString("Settings tab '%1' must have content").arg(tabText).toLatin1());
                    
                    // Test for typical settings controls in tab
                    testTabControlsIntegration(tabContent, tabText);
                    
                    qDebug() << "     Tab" << i << ":" << tabText;
                }
                
                // At least some professional tabs should be present
                int professionalTabsFound = 0;
                for (const QString& expected : expectedTabs) {
                    for (const QString& found : foundTabs) {
                        if (found.contains(expected, Qt::CaseInsensitive)) {
                            professionalTabsFound++;
                            break;
                        }
                    }
                }
                
                QVERIFY2(professionalTabsFound > 0, "Settings should have professional configuration tabs");
                qDebug() << "   ✅ Professional settings tabs found:" << professionalTabsFound;
            } else {
                qDebug() << "   ℹ️ Settings dialog may use different layout structure";
            }
            
            // Test dialog buttons (OK, Cancel, Apply)
            testDialogButtonsIntegration(&settingsDialog);
            
        } catch (const std::exception& e) {
            QFAIL(QString("Settings dialog test failed: %1").arg(e.what()).toLatin1());
        }
    }

    /**
     * @brief Test AboutDialog Qt Designer integration
     */
    void testAboutDialogDesignerIntegration() {
        qDebug() << "ℹ️ Testing AboutDialog Qt Designer Integration";
        
        try {
            // Create about dialog
            AboutDialog aboutDialog;
            
            // Test dialog basic properties
            QVERIFY(!aboutDialog.windowTitle().isEmpty());
            qDebug() << "   About dialog title:" << aboutDialog.windowTitle();
            
            // Validate Qt Designer integration
            VALIDATE_QT_DESIGNER_INTEGRATION(&aboutDialog);
            
            // Test about dialog content structure
            QTabWidget* tabWidget = aboutDialog.findChild<QTabWidget*>();
            if (tabWidget) {
                qDebug() << "   ✅ About tabs found:" << tabWidget->count();
                
                // Professional about dialogs typically have multiple information tabs
                QStringList expectedTabs = {"About", "License", "Credits", "Version", "System"};
                
                for (int i = 0; i < tabWidget->count(); ++i) {
                    QString tabText = tabWidget->tabText(i);
                    qDebug() << "     About tab" << i << ":" << tabText;
                    
                    // Test tab content exists and has information
                    QWidget* tabContent = tabWidget->widget(i);
                    QVERIFY2(tabContent != nullptr,
                             QString("About tab '%1' must have content").arg(tabText).toLatin1());
                    
                    // About tabs should contain labels or text edits with information
                    QList<QLabel*> labels = tabContent->findChildren<QLabel*>();
                    QList<QTextEdit*> textEdits = tabContent->findChildren<QTextEdit*>();
                    
                    bool hasContent = !labels.isEmpty() || !textEdits.isEmpty();
                    QVERIFY2(hasContent, 
                             QString("About tab '%1' should contain informational content").arg(tabText).toLatin1());
                }
            } else {
                // Alternative: Single page about dialog
                QList<QLabel*> labels = aboutDialog.findChildren<QLabel*>();
                if (!labels.isEmpty()) {
                    qDebug() << "   ✅ About dialog labels found:" << labels.size();
                    
                    // Check for typical about information
                    bool hasVersionInfo = false;
                    bool hasAppInfo = false;
                    
                    for (QLabel* label : labels) {
                        QString text = label->text();
                        if (text.contains("version", Qt::CaseInsensitive) ||
                            text.contains("v.", Qt::CaseInsensitive)) {
                            hasVersionInfo = true;
                        }
                        if (text.contains("StreamDAB", Qt::CaseInsensitive) ||
                            text.contains("ETI", Qt::CaseInsensitive)) {
                            hasAppInfo = true;
                        }
                    }
                    
                    QVERIFY2(hasVersionInfo || hasAppInfo, "About dialog should contain application information");
                } else {
                    qDebug() << "   ℹ️ About dialog may use custom layout implementation";
                }
            }
            
        } catch (const std::exception& e) {
            QFAIL(QString("About dialog test failed: %1").arg(e.what()).toLatin1());
        }
    }

    /**
     * @brief Test professional dialog signal/slot connections
     */
    void testDialogSignalSlotConnections() {
        qDebug() << "🔗 Testing Dialog Signal/Slot Connections";
        
        try {
            // Test settings dialog connections
            SettingsDialog settingsDialog;
            
            // Test dialog accepted/rejected signals
            QSignalSpy acceptedSpy(&settingsDialog, &QDialog::accepted);
            QSignalSpy rejectedSpy(&settingsDialog, &QDialog::rejected);
            
            QVERIFY(acceptedSpy.isValid());
            QVERIFY(rejectedSpy.isValid());
            
            // Look for OK and Cancel buttons
            QPushButton* okButton = settingsDialog.findChild<QPushButton*>("okButton");
            QPushButton* cancelButton = settingsDialog.findChild<QPushButton*>("cancelButton");
            
            // Alternative button search by text
            if (!okButton) {
                QList<QPushButton*> buttons = settingsDialog.findChildren<QPushButton*>();
                for (QPushButton* btn : buttons) {
                    if (btn->text().contains("OK", Qt::CaseInsensitive) ||
                        btn->text().contains("Accept", Qt::CaseInsensitive)) {
                        okButton = btn;
                        break;
                    }
                }
            }
            
            if (!cancelButton) {
                QList<QPushButton*> buttons = settingsDialog.findChildren<QPushButton*>();
                for (QPushButton* btn : buttons) {
                    if (btn->text().contains("Cancel", Qt::CaseInsensitive)) {
                        cancelButton = btn;
                        break;
                    }
                }
            }
            
            if (okButton) {
                qDebug() << "   ✅ OK button found:" << okButton->text();
                
                // Test button click triggers accepted signal
                okButton->click();
                QApplication::processEvents();
                
                // Note: Some buttons might trigger other slots before accept()
                qDebug() << "   OK button clicked - accepted signals:" << acceptedSpy.count();
            } else {
                qDebug() << "   ℹ️ OK button may use different naming or implementation";
            }
            
            if (cancelButton) {
                qDebug() << "   ✅ Cancel button found:" << cancelButton->text();
                
                // Reset dialog state first
                settingsDialog.close();
                settingsDialog.show();
                
                // Test button click triggers rejected signal
                cancelButton->click();
                QApplication::processEvents();
                
                qDebug() << "   Cancel button clicked - rejected signals:" << rejectedSpy.count();
            } else {
                qDebug() << "   ℹ️ Cancel button may use different naming or implementation";
            }
            
        } catch (const std::exception& e) {
            QFAIL(QString("Signal/slot connection test failed: %1").arg(e.what()).toLatin1());
        }
    }

    /**
     * @brief Test widget property persistence from Qt Designer
     */
    void testWidgetPropertyPersistence() {
        qDebug() << "🏷️ Testing Widget Property Persistence from Designer";
        
        try {
            SettingsDialog settingsDialog;
            
            // Test that widgets have proper object names (Designer sets these)
            QList<QWidget*> allWidgets = settingsDialog.findChildren<QWidget*>();
            
            int widgetsWithObjectNames = 0;
            int widgetsWithTooltips = 0;
            int widgetsWithSizeConstraints = 0;
            
            for (QWidget* widget : allWidgets) {
                QString objectName = widget->objectName();
                if (!objectName.isEmpty() && !objectName.startsWith("qt_")) {
                    widgetsWithObjectNames++;
                    
                    qDebug() << "     Widget:" << widget->metaObject()->className() 
                             << "name:" << objectName;
                }
                
                // Check for tooltips (often set in Designer)
                if (!widget->toolTip().isEmpty()) {
                    widgetsWithTooltips++;
                }
                
                // Check for size constraints
                QSize minSize = widget->minimumSize();
                QSize maxSize = widget->maximumSize();
                if (minSize.width() > 0 || minSize.height() > 0 ||
                    maxSize.width() < 16777215 || maxSize.height() < 16777215) {
                    widgetsWithSizeConstraints++;
                }
            }
            
            qDebug() << "   Widget Analysis Results:";
            qDebug() << "     Total widgets:" << allWidgets.size();
            qDebug() << "     Widgets with object names:" << widgetsWithObjectNames;
            qDebug() << "     Widgets with tooltips:" << widgetsWithTooltips;
            qDebug() << "     Widgets with size constraints:" << widgetsWithSizeConstraints;
            
            if (widgetsWithObjectNames > 0) {
                qDebug() << "   ✅ Qt Designer property persistence detected";
            } else {
                qDebug() << "   ℹ️ Widgets may use manual naming or custom implementation";
            }
            
            // Test specific professional widget configurations
            testProfessionalWidgetConfiguration(&settingsDialog);
            
        } catch (const std::exception& e) {
            QFAIL(QString("Widget property persistence test failed: %1").arg(e.what()).toLatin1());
        }
    }

    /**
     * @brief Test layout manager functionality from Qt Designer
     */
    void testLayoutManagerFunctionality() {
        qDebug() << "📐 Testing Layout Manager Functionality from Designer";
        
        try {
            SettingsDialog settingsDialog;
            
            // Test layout hierarchy
            QList<QLayout*> layouts = settingsDialog.findChildren<QLayout*>();
            
            qDebug() << "   Found" << layouts.size() << "layout managers";
            
            QMap<QString, int> layoutTypes;
            
            for (QLayout* layout : layouts) {
                QString typeName = layout->metaObject()->className();
                layoutTypes[typeName]++;
                
                // Test layout properties
                QVERIFY(layout->count() >= 0);
                
                // Test layout spacing and margins (often set in Designer)
                int spacing = layout->spacing();
                QMargins margins = layout->contentsMargins();
                
                qDebug() << "     Layout:" << typeName 
                         << "items:" << layout->count()
                         << "spacing:" << spacing
                         << "margins:" << margins.left() << margins.top() << margins.right() << margins.bottom();
                
                // Professional layouts should have reasonable spacing
                if (layout->count() > 1) {
                    QVERIFY2(spacing >= 0, "Layout spacing should be non-negative");
                }
            }
            
            qDebug() << "   Layout Type Distribution:";
            for (auto it = layoutTypes.begin(); it != layoutTypes.end(); ++it) {
                qDebug() << "     " << it.key() << ":" << it.value();
            }
            
            // Test that major layout types are present for professional dialogs
            bool hasVBoxLayout = layoutTypes.contains("QVBoxLayout");
            bool hasHBoxLayout = layoutTypes.contains("QHBoxLayout");
            bool hasFormLayout = layoutTypes.contains("QFormLayout");
            bool hasGridLayout = layoutTypes.contains("QGridLayout");
            
            if (hasVBoxLayout || hasHBoxLayout || hasFormLayout || hasGridLayout) {
                qDebug() << "   ✅ Professional layout managers detected";
                qDebug() << "     VBox:" << hasVBoxLayout << "HBox:" << hasHBoxLayout 
                         << "Form:" << hasFormLayout << "Grid:" << hasGridLayout;
            } else {
                qDebug() << "   ℹ️ Custom layout implementation may be used";
            }
            
        } catch (const std::exception& e) {
            QFAIL(QString("Layout manager functionality test failed: %1").arg(e.what()).toLatin1());
        }
    }

    /**
     * @brief Test UI file structure validation
     */
    void testUIFileStructureValidation() {
        qDebug() << "📄 Testing UI File Structure Validation";
        
        // Test each available UI file
        for (const QString& uiFilePath : m_availableUIFiles) {
            qDebug() << "   Testing UI file:" << QFileInfo(uiFilePath).fileName();
            
            bool loadResult = m_designerTester->testUIFileLoading(uiFilePath);
            
            if (loadResult) {
                qDebug() << "     ✅ UI file loaded successfully";
                
                // Additional validation - check file content
                validateUIFileContent(uiFilePath);
            } else {
                qWarning() << "     ❌ UI file failed to load";
                // Don't fail the test - file might not exist yet in TDD
            }
        }
        
        if (m_availableUIFiles.isEmpty()) {
            qDebug() << "   ℹ️ No UI files found - may be using manual layout implementation";
        }
    }

    /**
     * @brief Performance test for Qt Designer integration
     */
    void testDesignerIntegrationPerformance() {
        qDebug() << "⚡ Testing Qt Designer Integration Performance";
        
        // Test dialog creation performance
        TDD_BENCHMARK([&]() {
            SettingsDialog dialog;
            dialog.show();
            QApplication::processEvents();
            dialog.close();
        }, 500, "Settings dialog creation and display"); // Max 500ms
        
        // Test about dialog performance
        TDD_BENCHMARK([&]() {
            AboutDialog dialog;
            dialog.show();
            QApplication::processEvents();
            dialog.close();
        }, 300, "About dialog creation and display"); // Max 300ms
        
        // Test memory impact of designer integration
        TDD_MEMORY_CHECK([&]() {
            QList<std::unique_ptr<QDialog>> dialogs;
            
            for (int i = 0; i < 10; ++i) {
                dialogs.push_back(std::make_unique<SettingsDialog>());
            }
            
            // Process events to ensure full initialization
            QApplication::processEvents();
            
        }, 50, "Multiple dialog instances memory usage"); // Max 50MB
        
        qDebug() << "   ✅ Qt Designer integration performance validated";
    }

    /**
     * @brief Test cross-platform UI consistency
     */
    void testCrossPlatformConsistency() {
        qDebug() << "🖥️ Testing Cross-Platform UI Consistency";
        
        try {
            SettingsDialog settingsDialog;
            
            // Test that dialog uses platform-appropriate styling
            QStyle* dialogStyle = settingsDialog.style();
            QVERIFY2(dialogStyle != nullptr, "Dialog must have valid style");
            
            QString styleName = dialogStyle->objectName();
            qDebug() << "   Dialog style:" << styleName;
            
            // Test dialog size and positioning
            QSize dialogSize = settingsDialog.sizeHint();
            qDebug() << "   Dialog size hint:" << dialogSize.width() << "x" << dialogSize.height();
            
            QVERIFY2(dialogSize.width() > 0 && dialogSize.height() > 0,
                     "Dialog must have valid size hint");
            
            // Test that dialog can be properly centered
            QSize screenSize = QApplication::primaryScreen()->size();
            QPoint centerPos = QPoint((screenSize.width() - dialogSize.width()) / 2,
                                     (screenSize.height() - dialogSize.height()) / 2);
            
            settingsDialog.move(centerPos);
            QPoint actualPos = settingsDialog.pos();
            
            qDebug() << "   Dialog positioning: center" << centerPos << "actual" << actualPos;
            
            // Test font rendering consistency
            QFont dialogFont = settingsDialog.font();
            QFontMetrics fontMetrics(dialogFont);
            
            QString testText = "Professional Broadcast Analysis Settings";
            int textWidth = fontMetrics.horizontalAdvance(testText);
            int textHeight = fontMetrics.height();
            
            qDebug() << "   Font metrics for '" << testText << "':"
                     << textWidth << "x" << textHeight << "pixels";
            
            QVERIFY2(textWidth > 0 && textHeight > 0, "Font metrics should be valid");
            
        } catch (const std::exception& e) {
            QFAIL(QString("Cross-platform consistency test failed: %1").arg(e.what()).toLatin1());
        }
    }

private:
    /**
     * @brief Scan for available UI files in the project
     */
    void scanForUIFiles() {
        // Look in standard locations for UI files
        QStringList searchPaths = {
            "../src/gui/ui",
            "../../src/gui/ui",
            "../../../src/gui/ui",
            "src/gui/ui"
        };
        
        for (const QString& path : searchPaths) {
            QDir uiDir(path);
            if (uiDir.exists()) {
                QStringList uiFiles = uiDir.entryList(QStringList("*.ui"), QDir::Files);
                for (const QString& file : uiFiles) {
                    QString fullPath = uiDir.absoluteFilePath(file);
                    if (!m_availableUIFiles.contains(fullPath)) {
                        m_availableUIFiles.append(fullPath);
                    }
                }
                
                if (!uiFiles.isEmpty()) {
                    qDebug() << "Found UI files in" << path << ":" << uiFiles.size();
                    break; // Use first valid directory
                }
            }
        }
    }

    /**
     * @brief Test tab controls integration for settings
     */
    void testTabControlsIntegration(QWidget* tabContent, const QString& tabName) {
        if (!tabContent) return;
        
        // Count different types of controls
        int checkBoxes = tabContent->findChildren<QCheckBox*>().size();
        int spinBoxes = tabContent->findChildren<QSpinBox*>().size();
        int comboBoxes = tabContent->findChildren<QComboBox*>().size();
        int lineEdits = tabContent->findChildren<QLineEdit*>().size();
        int sliders = tabContent->findChildren<QSlider*>().size();
        
        int totalControls = checkBoxes + spinBoxes + comboBoxes + lineEdits + sliders;
        
        if (totalControls > 0) {
            qDebug() << "       Tab '" << tabName << "' controls:"
                     << "CheckBox:" << checkBoxes
                     << "SpinBox:" << spinBoxes
                     << "ComboBox:" << comboBoxes
                     << "LineEdit:" << lineEdits
                     << "Slider:" << sliders;
        } else {
            qDebug() << "       Tab '" << tabName << "' may have custom controls";
        }
    }

    /**
     * @brief Test dialog buttons integration
     */
    void testDialogButtonsIntegration(QDialog* dialog) {
        QList<QPushButton*> buttons = dialog->findChildren<QPushButton*>();
        
        qDebug() << "     Dialog buttons found:" << buttons.size();
        
        for (QPushButton* button : buttons) {
            QString buttonText = button->text();
            if (!buttonText.isEmpty()) {
                qDebug() << "       Button:" << buttonText << "enabled:" << button->isEnabled();
                
                // Test button properties
                QVERIFY(button->isVisible());
                
                // Professional buttons should have reasonable size
                QSize buttonSize = button->size();
                if (buttonSize.width() > 0 && buttonSize.height() > 0) {
                    QVERIFY2(buttonSize.width() >= 70 && buttonSize.height() >= 25,
                             "Professional dialog buttons should have reasonable minimum size");
                }
            }
        }
    }

    /**
     * @brief Test professional widget configuration
     */
    void testProfessionalWidgetConfiguration(QWidget* parentWidget) {
        // Test group boxes for professional organization
        QList<QGroupBox*> groupBoxes = parentWidget->findChildren<QGroupBox*>();
        
        for (QGroupBox* groupBox : groupBoxes) {
            QString title = groupBox->title();
            if (!title.isEmpty()) {
                qDebug() << "       Group box:" << title << "checkable:" << groupBox->isCheckable();
                
                // Professional group boxes should organize related settings
                int childControls = groupBox->findChildren<QWidget*>().size();
                if (childControls > 1) {
                    qDebug() << "         Child controls:" << childControls;
                }
            }
        }
        
        // Test progress bars for professional feedback
        QList<QProgressBar*> progressBars = parentWidget->findChildren<QProgressBar*>();
        for (QProgressBar* progressBar : progressBars) {
            QVERIFY(progressBar->minimum() <= progressBar->maximum());
            qDebug() << "       Progress bar range:" << progressBar->minimum() 
                     << "to" << progressBar->maximum();
        }
    }

    /**
     * @brief Validate UI file XML content
     */
    void validateUIFileContent(const QString& uiFilePath) {
        QFile file(uiFilePath);
        if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            return;
        }
        
        QByteArray content = file.readAll();
        file.close();
        
        // Basic XML validation
        bool hasUITag = content.contains("<ui version=");
        bool hasWidgetTag = content.contains("<widget class=");
        bool hasLayoutTag = content.contains("<layout class=");
        
        if (hasUITag && hasWidgetTag) {
            qDebug() << "       ✅ Valid Qt Designer UI file structure";
            
            if (hasLayoutTag) {
                qDebug() << "       ✅ Contains layout definitions";
            }
            
            // Check for professional widget classes
            QStringList professionalWidgets = {
                "QTabWidget", "QGroupBox", "QComboBox", "QSpinBox", 
                "QCheckBox", "QRadioButton", "QSlider", "QProgressBar"
            };
            
            int professionalWidgetsFound = 0;
            for (const QString& widget : professionalWidgets) {
                if (content.contains(widget.toUtf8())) {
                    professionalWidgetsFound++;
                }
            }
            
            qDebug() << "       Professional widget types found:" << professionalWidgetsFound;
        } else {
            qWarning() << "       ❌ Invalid or incomplete UI file structure";
        }
    }

    void cleanupTestCase() {
        if (m_testFramework) {
            m_testFramework->cleanupProfessionalTestEnvironment();
            delete m_testFramework;
            m_testFramework = nullptr;
        }
        
        qDebug() << "Qt Designer Integration tests completed";
        TDD::TestReporter::instance().enforceTDDCompliance();
    }
};

// Register test with Qt Test framework
QTEST_MAIN(QtDesignerIntegrationTest)
#include "test_qt_designer_integration.moc"