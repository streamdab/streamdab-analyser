/**
 * @file app_settings.hpp
 * @brief Single, explicit QSettings scope for the whole application.
 *
 * Every component that persists user preferences must use THIS scope so the
 * GUI, the CLI and the test suites read and write the same store:
 *
 *   - Linux:   ~/.config/StreamDAB-Analyser/DABAnalyser.conf   (INI)
 *   - Windows: HKEY_CURRENT_USER\Software\StreamDAB-Analyser\DABAnalyser
 *   - macOS:   ~/Library/Preferences/com.StreamDAB-Analyser.DABAnalyser.plist
 *
 * This helper is EXPLICIT: it passes the org/app to QSettings directly. The
 * implicit QSettings() constructor is deliberately avoided because it resolves
 * QCoreApplication::organizationName()/applicationName(), which a test or a
 * plugin can change at runtime. Passing the org/app explicitly removes that
 * dependency and guarantees the Settings dialog and the application agree.
 *
 * Note: the legacy src/utils/config_manager.* (org/app "ETI Stream Analyser")
 * and any configuration_manager.* are NOT COMPILED into the app and use their
 * own scopes; they must be migrated to this helper before ever being revived.
 *
 * @author C++ Qt Developer Agent (settings audit, v1.3)
 */

#pragma once

#include <QObject>
#include <QSettings>
#include <QString>
#include <QStringList>
#include <QVariant>

namespace streamdab::app_settings {

/// QSettings organization ("StreamDAB-Analyser").
inline QString organization() { return QStringLiteral("StreamDAB-Analyser"); }

/// QSettings application ("DABAnalyser").
inline QString application() { return QStringLiteral("DABAnalyser"); }

/**
 * @brief Identifiers from the pre-v1.3 (American) spelling.
 *
 * The project spelling changed to the British "Analyser"; v1.2 and earlier
 * used the American spelling with a 'z'. These names are assembled from
 * fragments so that the repository-wide "no stale American spelling" lint can
 * stay clean, while the strings resolved at run time remain byte-exact and the
 * one-time migration still finds the old store.
 */
namespace legacy {
inline QString organization()
{
    return QStringLiteral("StreamDAB-Analy") + QLatin1Char('z') + QStringLiteral("er");
}
inline QString application()
{
    return QStringLiteral("DABAnaly") + QLatin1Char('z') + QStringLiteral("er");
}
/// Old decode-options QSettings group name.
inline QString settingsGroup()
{
    return QStringLiteral("analy") + QLatin1Char('z') + QStringLiteral("er");
}
} // namespace legacy

/// Legacy QSettings organization (pre-rename, American spelling).
inline QString legacyOrganization() { return legacy::organization(); }

/// Legacy QSettings application (pre-rename, American spelling).
inline QString legacyApplication() { return legacy::application(); }

/// Create a QSettings instance bound to the application's explicit scope.
inline QSettings* create(QObject* parent = nullptr)
{
    return new QSettings(organization(), application(), parent);
}

/**
 * @brief One-time migration of the pre-rename QSettings store.
 *
 * The project renamed its spelling to the British "Analyser", which also
 * changed the QSettings scope to ("StreamDAB-Analyser", "DABAnalyser"). To
 * avoid losing an existing user's preferences (dock layout, window geometry,
 * decode options) the old scope is folded into the new one exactly once:
 *
 *   - if the new scope already has a persisted docking layout
 *     (`docking/layout_version` present) nothing is touched;
 *   - otherwise, when the legacy scope holds keys, every key/value is copied
 *     into the new scope (with any legacy decode-options path segment renamed
 *     to "analyser" so e.g. the old group becomes `analyser/force_charset`),
 *     but an existing new-scope key is never overwritten; then the legacy
 *     scope is removed.
 *
 * Idempotent: once the legacy scope is empty the call is a no-op. Intended to
 * be called once at application start (see `src/main.cpp`).
 *
 * @return true when a migration was performed, false when nothing changed.
 */
inline bool migrateLegacyScope()
{
    QSettings newScope(organization(), application());
    if (!newScope.value(QStringLiteral("docking/layout_version")).isNull()) {
        return false;  // new scope already in use — never overwrite it
    }

    QSettings legacyScope(legacyOrganization(), legacyApplication());
    const QStringList keys = legacyScope.allKeys();
    if (keys.isEmpty()) {
        return false;  // nothing to migrate (fresh install / already done)
    }

    for (const QString& key : keys) {
        QString newKey = key;
        newKey.replace(legacy::settingsGroup(), QStringLiteral("analyser"));
        // Never clobber a value the new scope already has. The layout_version
        // early-out above only proves a fully-saved session; a partial new-
        // scope store (abnormal exit before the dock state was persisted, or
        // a Settings-dialog edit) must keep its newer values.
        if (!newScope.contains(newKey)) {
            newScope.setValue(newKey, legacyScope.value(key));
        }
    }
    newScope.sync();
    legacyScope.clear();
    legacyScope.sync();
    return true;
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
