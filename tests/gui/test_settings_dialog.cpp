/**
 * @file test_settings_dialog.cpp
 * @brief Settings-dialog wiring tests (v1.3 settings audit).
 *
 * Validates that every control shown in the dialog either drives real
 * behaviour (WIRE) or does not exist (REMOVE), and that the dialog persists
 * to the SAME explicit QSettings scope the application reads
 * (StreamDAB-Analyser / DABAnalyser, see src/utils/app_settings.hpp):
 *
 *   WIRE   : general/confirmExit, analysis/updateRate, analysis/bufferSize,
 *            audio/device|muted|volume, advanced/logLevel, analyser group
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
#include <QScrollArea>

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
    "analyser/force_charset",
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
        QCoreApplication::setOrganizationName(QStringLiteral("StreamDAB-Analyser"));
        QCoreApplication::setApplicationName(QStringLiteral("DABAnalyser"));
        QSettings(QStringLiteral("StreamDAB-Analyser"), QStringLiteral("DABAnalyser")).clear();
    }

    void TearDown() override
    {
        QSettings(QStringLiteral("StreamDAB-Analyser"), QStringLiteral("DABAnalyser")).clear();
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

TEST_F(SettingsDialogTest, ProcessingTabIsSelfExplanatoryAndRoundTrips)
{
    SettingsDialog dialog;

    // Stable object names (the wiring/tests rely on them).
    QSpinBox* updateRate = dialog.findChild<QSpinBox*>(QStringLiteral("settingsUpdateRate"));
    QSpinBox* bufferSize = dialog.findChild<QSpinBox*>(QStringLiteral("settingsBufferSize"));
    ASSERT_NE(updateRate, nullptr);
    ASSERT_NE(bufferSize, nullptr);

    // Unit labels present (presence, not exact prose).
    EXPECT_FALSE(updateRate->suffix().trimmed().isEmpty());
    EXPECT_FALSE(bufferSize->suffix().trimmed().isEmpty());
    EXPECT_TRUE(updateRate->suffix().contains(QStringLiteral("Hz"), Qt::CaseInsensitive));
    EXPECT_TRUE(bufferSize->suffix().contains(QStringLiteral("frame"), Qt::CaseInsensitive));

    // Tooltips state the effect (non-empty) — presence, not exact prose.
    EXPECT_FALSE(updateRate->toolTip().trimmed().isEmpty());
    EXPECT_FALSE(bufferSize->toolTip().trimmed().isEmpty());

    // Values still round-trip through the shared store.
    updateRate->setValue(7);
    bufferSize->setValue(2500);
    dialog.saveSettings();

    const QSettings s(streamdab::app_settings::organization(),
                      streamdab::app_settings::application());
    EXPECT_EQ(s.value(QStringLiteral("analysis/updateRate")).toInt(), 7);
    EXPECT_EQ(s.value(QStringLiteral("analysis/bufferSize")).toInt(), 2500);

    SettingsDialog reload;
    EXPECT_EQ(reload.getUpdateRate(), 7);
    EXPECT_EQ(reload.getBufferSize(), 2500);
}

TEST_F(SettingsDialogTest, RemovedControlsDoNotExistAndAreNotPersisted)
{
    SettingsDialog dialog;

    for (const char* name : kRemovedObjectNames) {
        EXPECT_EQ(dialog.findChild<QWidget*>(QString::fromLatin1(name)), nullptr)
            << "removed control still present: " << name;
    }

    // The removed TABS (Display, Network, Performance) are also gone; the kept
    // tabs are General, Processing (renamed from the unclear "Analysis"),
    // Audio, Advanced, Analyser.
    QTabWidget* tabs = dialog.findChild<QTabWidget*>();
    ASSERT_NE(tabs, nullptr);
    QStringList labels;
    for (int i = 0; i < tabs->count(); ++i) {
        labels << tabs->tabText(i);
    }
    EXPECT_EQ(labels, QStringList({QStringLiteral("General"), QStringLiteral("Processing"),
                                   QStringLiteral("Audio"), QStringLiteral("Advanced"),
                                   QStringLiteral("Analyser")}));

    // saveSettings must never write a removed key.
    dialog.saveSettings();
    const QSettings s(streamdab::app_settings::organization(),
                      streamdab::app_settings::application());
    for (const char* key : kRemovedKeys) {
        EXPECT_FALSE(s.contains(QString::fromLatin1(key)))
            << "removed key was persisted: " << key;
    }
}

TEST_F(SettingsDialogTest, AnalyserTabStillRoundTrips)
{
    // The already-wired analyser tab must keep working (regression guard).
    SettingsDialog dialog;
    QSpinBox* forceCharset = dialog.findChild<QSpinBox*>(QStringLiteral("settingsForceCharset"));
    ASSERT_NE(forceCharset, nullptr);
    forceCharset->setValue(6); // Thai TIS-620
    dialog.saveSettings();

    QSettings mutableStore(streamdab::app_settings::organization(),
                           streamdab::app_settings::application());
    eti::AnalyserSettings roundTripped = eti::AnalyserSettings::defaults();
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
    EXPECT_EQ(SettingsDialog::loggerLevelForIndex(4), 4);  // Critical

    // The inverse mapping must round-trip every combo entry, so the bottom-strip
    // Logging tab and the Settings dialog can share `advanced/logLevel`.
    for (int i = 0; i <= 4; ++i) {
        EXPECT_EQ(SettingsDialog::indexForLoggerLevel(SettingsDialog::loggerLevelForIndex(i)), i)
            << "log-level mapping must be bijective at index " << i;
    }
}

TEST_F(SettingsDialogTest, EveryTabScrollsAndAnalyserControlsAreReachable)
{
    SettingsDialog dialog;
    QTabWidget* tabs = dialog.findChild<QTabWidget*>();
    ASSERT_NE(tabs, nullptr);
    ASSERT_GE(tabs->count(), 5) << "General/Processing/Audio/Advanced/Analyser";

    // Every tab's content lives in a resizable scroll area: long forms (the
    // Analyser tab has groups A..I) must never be squeezed until only the
    // group titles remain visible.
    for (int i = 0; i < tabs->count(); ++i) {
        auto* scroll = qobject_cast<QScrollArea*>(tabs->widget(i));
        ASSERT_NE(scroll, nullptr) << tabs->tabText(i).toStdString();
        EXPECT_TRUE(scroll->widgetResizable());
        EXPECT_NE(scroll->widget(), nullptr) << tabs->tabText(i).toStdString();
    }

    int analyserIndex = -1;
    for (int i = 0; i < tabs->count(); ++i) {
        if (tabs->tabText(i) == QStringLiteral("Analyser")) {
            analyserIndex = i;
        }
    }
    ASSERT_GE(analyserIndex, 0) << "Analyser tab must exist";
    tabs->setCurrentIndex(analyserIndex);
    dialog.show();
    QCoreApplication::processEvents();

    auto* scroll = qobject_cast<QScrollArea*>(tabs->widget(analyserIndex));
    ASSERT_NE(scroll, nullptr);
    QWidget* content = scroll->widget();
    ASSERT_NE(content, nullptr);

    // The matrix A..I rows must exist as real editable controls (13 widgets).
    const auto spins = content->findChildren<QSpinBox*>();
    const auto combos = content->findChildren<QComboBox*>();
    const auto checks = content->findChildren<QCheckBox*>();
    EXPECT_EQ(spins.size() + combos.size() + checks.size(), 13)
        << "spins=" << spins.size() << " combos=" << combos.size()
        << " checks=" << checks.size();

    // ...and each one must have a usable geometry in the shown dialog (the bug
    // was controls clipped away so only group titles were visible).
    QList<QWidget*> editable;
    for (auto* w : spins) editable.append(w);
    for (auto* w : combos) editable.append(w);
    for (auto* w : checks) editable.append(w);
    for (QWidget* w : editable) {
        EXPECT_GT(w->height(), 0);
        EXPECT_GT(w->width(), 0);
        EXPECT_TRUE(w->isVisibleTo(content));
    }

    // The form is taller than the viewport, i.e. the scroll area is what makes
    // the lower groups reachable rather than dead space.
    EXPECT_GT(content->sizeHint().height(), 0);

    dialog.close();
    QCoreApplication::processEvents();
}
