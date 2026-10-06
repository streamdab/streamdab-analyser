/**
 * @file test_settings_dialog.cpp
 * @brief Settings-dialog wiring tests (v1.3 settings audit).
 *
 * Validates that every control shown in the dialog either drives real
 * behaviour (WIRE) or does not exist (REMOVE), and that the dialog persists
 * to the SAME explicit QSettings scope the application reads
 * (StreamDAB-Analyzer / DABAnalyzer, see src/utils/app_settings.hpp):
 *
 *   WIRE   : general/confirmExit, analysis/updateRate, analysis/bufferSize,
 *            audio/device|muted|volume, advanced/logLevel, analyzer group
 *   REMOVE : general/theme|autoSave|autoSaveInterval, display group,
 *            analysis/realTime|hardwareAccel, network group, performance group,
 *            advanced/debugMode — no widget, no persisted key.
 *
 * @author C++ Qt Developer Agent (settings audit, v1.3)
 */

#include <gtest/gtest.h>
#include <QApplication>
#include <QSettings>
#include <QStandardPaths>
#include <QSpinBox>
#include <QComboBox>
#include <QCheckBox>
#include <QSlider>
#include <QLabel>
#include <QLineEdit>
#include <QTabWidget>

#include "gui/settings_dialog.h"
#include "utils/app_settings.hpp"

namespace {

// Parallel with the audited key groups in docs/CONFIGURATION.md.
const char* const kWiredKeys[] = {
    "general/confirmExit",
    "analysis/updateRate",
    "analysis/bufferSize",
    "audio/device",
    "audio/muted",
    "audio/volume",
    "advanced/logLevel",
    "analyzer/force_charset",
};

// Keys that were REMOVEd: nobody may write or read them any more.
const char* const kRemovedKeys[] = {
    "general/theme",
    "general/autoSave",
    "general/autoSaveInterval",
    "analysis/realTime",
    "analysis/hardwareAccel",
    "display/backgroundColor",
    "display/gridColor",
    "display/textColor",
    "display/font",
    "network/useProxy",
    "network/proxyHost",
    "network/proxyPort",
    "performance/memoryLimit",
    "performance/priority",
    "advanced/debugMode",
};

// Widget object names that must no longer exist (REMOVEd controls).
const char* const kRemovedObjectNames[] = {
    "settingsTheme",
    "settingsAutoSave",
    "settingsAutoSaveInterval",
    "settingsMemoryLimit",
    "settingsPriority",
    "settingsProxyHost",
    "settingsProxyPort",
    "settingsHardwareAccel",
    "settingsRealTime",
    "settingsDebugMode",
    "settingsBackgroundColor",
    "settingsFont",
};

class SettingsDialogTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        if (!QApplication::instance()) {
            int argc = 0;
            char** argv = nullptr;
            app = new QApplication(argc, argv);
        }
        // Isolate from the real user store.
        QStandardPaths::setTestModeEnabled(true);
        QCoreApplication::setOrganizationName(QStringLiteral("StreamDAB-Analyzer"));
        QCoreApplication::setApplicationName(QStringLiteral("DABAnalyzer"));
        QSettings(QStringLiteral("StreamDAB-Analyzer"), QStringLiteral("DABAnalyzer")).clear();
    }

    void TearDown() override
    {
        QSettings(QStringLiteral("StreamDAB-Analyzer"), QStringLiteral("DABAnalyzer")).clear();
    }

    QApplication* app = nullptr;
};

} // namespace

TEST_F(SettingsDialogTest, ConfigScopeMatchesApplication)
{
    SettingsDialog dialog;
    // The dialog and the application must agree on one explicit QSettings store.
    EXPECT_EQ(streamdab::app_settings::filePath(),
              QSettings(streamdab::app_settings::organization(),
                        streamdab::app_settings::application()).fileName());
    QLabel* pathLabel = dialog.findChild<QLabel*>(QStringLiteral("settingsConfigPath"));
    ASSERT_NE(pathLabel, nullptr);
    EXPECT_EQ(pathLabel->text(), streamdab::app_settings::filePath());
    EXPECT_FALSE(streamdab::app_settings::filePath().isEmpty());
}

TEST_F(SettingsDialogTest, WiredControlsPersistAndRoundTrip)
{
    SettingsDialog dialog;

    QSpinBox* updateRate = dialog.findChild<QSpinBox*>(QStringLiteral("settingsUpdateRate"));
    QSpinBox* bufferSize = dialog.findChild<QSpinBox*>(QStringLiteral("settingsBufferSize"));
    QCheckBox* confirmExit = dialog.findChild<QCheckBox*>(QStringLiteral("settingsConfirmExit"));
    QComboBox* logLevel = dialog.findChild<QComboBox*>(QStringLiteral("settingsLogLevel"));
    ASSERT_NE(updateRate, nullptr);
    ASSERT_NE(bufferSize, nullptr);
    ASSERT_NE(confirmExit, nullptr);
    ASSERT_NE(logLevel, nullptr);

    // Distinct, non-default values.
    updateRate->setValue(5);
    bufferSize->setValue(5000);
    confirmExit->setChecked(false);
    logLevel->setCurrentIndex(3); // Debug

    // Audio: leave the combo empty (dialog without a backend) — the selected
    // device id still persists from setAudioOptions state.
    dialog.setAudioOptions(QStringList{} << QStringLiteral("Default") << QStringLiteral("HDMI"),
                           QStringList{} << QStringLiteral("default") << QStringLiteral("hw:0,3"),
                           QStringLiteral("hw:0,3"), true, 40);
    EXPECT_EQ(dialog.audioOutputDevice(), QStringLiteral("hw:0,3"));
    EXPECT_TRUE(dialog.isAudioMuted());
    EXPECT_EQ(dialog.audioVolume(), 40);

    dialog.saveSettings();

    const QSettings s(streamdab::app_settings::organization(),
                      streamdab::app_settings::application());
    EXPECT_EQ(s.value(QStringLiteral("general/confirmExit")).toBool(), false);
    EXPECT_EQ(s.value(QStringLiteral("analysis/updateRate")).toInt(), 5);
    EXPECT_EQ(s.value(QStringLiteral("analysis/bufferSize")).toInt(), 5000);
    EXPECT_EQ(s.value(QStringLiteral("advanced/logLevel")).toInt(), 3);
    EXPECT_EQ(s.value(QStringLiteral("audio/device")).toString(), QStringLiteral("hw:0,3"));
    EXPECT_EQ(s.value(QStringLiteral("audio/muted")).toBool(), true);
    EXPECT_EQ(s.value(QStringLiteral("audio/volume")).toInt(), 40);

    // A fresh dialog loads the persisted values back.
    SettingsDialog reload;
    EXPECT_EQ(reload.getUpdateRate(), 5);
    EXPECT_EQ(reload.getBufferSize(), 5000);
    EXPECT_FALSE(reload.isConfirmExitEnabled());
    EXPECT_EQ(reload.logLevelIndex(), 3);
    EXPECT_TRUE(reload.isAudioMuted());
    EXPECT_EQ(reload.audioVolume(), 40);
}

TEST_F(SettingsDialogTest, RemovedControlsDoNotExistAndAreNotPersisted)
{
    SettingsDialog dialog;

    for (const char* name : kRemovedObjectNames) {
        EXPECT_EQ(dialog.findChild<QWidget*>(QString::fromLatin1(name)), nullptr)
            << "removed control still present: " << name;
    }

    // The removed TABS (Display, Network, Performance) are also gone; the kept
    // tabs are General, Analysis, Audio, Advanced, Analyzer.
    QTabWidget* tabs = dialog.findChild<QTabWidget*>();
    ASSERT_NE(tabs, nullptr);
    QStringList labels;
    for (int i = 0; i < tabs->count(); ++i) {
        labels << tabs->tabText(i);
    }
    EXPECT_EQ(labels, QStringList({QStringLiteral("General"), QStringLiteral("Analysis"),
                                   QStringLiteral("Audio"), QStringLiteral("Advanced"),
                                   QStringLiteral("Analyzer")}));

    // saveSettings must never write a removed key.
    dialog.saveSettings();
    const QSettings s(streamdab::app_settings::organization(),
                      streamdab::app_settings::application());
    for (const char* key : kRemovedKeys) {
        EXPECT_FALSE(s.contains(QString::fromLatin1(key)))
            << "removed key was persisted: " << key;
    }
}

TEST_F(SettingsDialogTest, AnalyzerTabStillRoundTrips)
{
    // The already-wired analyzer tab must keep working (regression guard).
    SettingsDialog dialog;
    QSpinBox* forceCharset = dialog.findChild<QSpinBox*>(QStringLiteral("settingsForceCharset"));
    ASSERT_NE(forceCharset, nullptr);
    forceCharset->setValue(6); // Thai TIS-620
    dialog.saveSettings();

    QSettings mutableStore(streamdab::app_settings::organization(),
                           streamdab::app_settings::application());
    eti::AnalyzerSettings roundTripped = eti::AnalyzerSettings::defaults();
    roundTripped.loadFromQSettings(mutableStore);
    EXPECT_EQ(roundTripped.force_charset, 6);
}

TEST_F(SettingsDialogTest, LoggerLevelMapping)
{
    // Combo index -> Logger::LogLevel integer (Debug=0, Info=1, Warning=2,
    // Error=3, Critical=4).
    EXPECT_EQ(SettingsDialog::loggerLevelForIndex(0), 3);  // Error
    EXPECT_EQ(SettingsDialog::loggerLevelForIndex(1), 2);  // Warning
    EXPECT_EQ(SettingsDialog::loggerLevelForIndex(2), 1);  // Info (default)
    EXPECT_EQ(SettingsDialog::loggerLevelForIndex(3), 0);  // Debug
}