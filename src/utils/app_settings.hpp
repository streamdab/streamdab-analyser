/**
 * @file app_settings.hpp
 * @brief Single, explicit QSettings scope for the whole application.
 *
 * Every component that persists user preferences must use THIS scope so the
 * GUI, the CLI and the test suites read and write the same store:
 *
 *   - Linux:   ~/.config/StreamDAB-Analyzer/DABAnalyzer.conf   (INI)
 *   - Windows: HKEY_CURRENT_USER\Software\StreamDAB-Analyzer\DABAnalyzer
 *   - macOS:   ~/Library/Preferences/com.StreamDAB-Analyzer.DABAnalyzer.plist
 *
 * This helper is EXPLICIT: it passes the org/app to QSettings directly. The
 * implicit QSettings() constructor is deliberately avoided because it resolves
 * QCoreApplication::organizationName()/applicationName(), which a test or a
 * plugin can change at runtime. Passing the org/app explicitly removes that
 * dependency and guarantees the Settings dialog and the application agree.
 *
 * Note: the legacy src/utils/config_manager.* (org/app "ETI Stream Analyzer")
 * and any configuration_manager.* are NOT COMPILED into the app and use their
 * own scopes; they must be migrated to this helper before ever being revived.
 *
 * @author C++ Qt Developer Agent (settings audit, v1.3)
 */

#pragma once

#include <QObject>
#include <QSettings>
#include <QString>

namespace streamdab::app_settings {

/// QSettings organization ("StreamDAB-Analyzer").
inline QString organization() { return QStringLiteral("StreamDAB-Analyzer"); }

/// QSettings application ("DABAnalyzer").
inline QString application() { return QStringLiteral("DABAnalyzer"); }

/// Create a QSettings instance bound to the application's explicit scope.
inline QSettings* create(QObject* parent = nullptr)
{
    return new QSettings(organization(), application(), parent);
}

/**
 * @brief Native per-platform store path (shown in the Settings dialog and
 *        docs/CONFIGURATION.md).
 */
inline QString filePath()
{
    QSettings s(organization(), application());
    return s.fileName();
}

} // namespace streamdab::app_settings
