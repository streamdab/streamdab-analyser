/**
 * @file test_gui_dock_layout.cpp
 * @brief Qt-ADS (Qt Advanced Docking System) layout tests for DABAnalyzerWindow
 *
 * v1.3 GUI migration validation:
 *   a) exactly 4 CDockManagers (one per tab + one window-level bottom dock area)
 *   b) exactly 26 dock widgets with the expected objectNames
 *   c) default first-launch placement: 4 left / 6 center / 6 right columns on
 *      Tab 1, 5 tabbed docks in the bottom manager
 *   d) QSettings persistence round-trip (closed dock stays closed; restored
 *      column structure matches the default layout)
 *   e) auto-hide (pin) config sanity
 *   f) floating round-trip safety (float a dock, re-dock it, no crash)
 *
 * Compiles the REAL application: src/main.cpp is included with GUI_TEST_MODE
 * (which excludes main()); the rest of ${SOURCES} is compiled by the target.
 *
 * @date October 2026
 */

#include <QtTest/QtTest>
#include <QStandardPaths>
#include <QSettings>
#include <QTabWidget>
#include <QSplitter>
#include <QGroupBox>

#include "DockManager.h"
#include "DockWidget.h"
#include "DockAreaWidget.h"

// Include the real application window implementation (main() excluded via
// GUI_TEST_MODE compile definition). The class and all panels come from here.
#include "../src/main.cpp"

class TestGuiDockLayout : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanupTestCase();
    void testFourDockManagers();
    void test26DocksWithExpectedNames();
    void testDefaultColumnPlacement();
    void testBottomFiveDocks();
    void testPersistenceRoundTrip();
    void testFloatingRoundTrip();
    void testAutoHideConfig();

private:
    // Removes every DABAnalyzerWindow QSettings key used by the app so each
    // window-based test starts from a CLEARED store. window.close() saves dock
    // state; without this, the next window would restore the previous layout
    // and break first-launch expectations.
    static void clearSettingsStore()
    {
        QSettings s(QStringLiteral("StreamDAB-Analyzer"), QStringLiteral("DABAnalyzer"));
        s.remove(QStringLiteral("window/geometry"));
        s.remove(QStringLiteral("docking/tab1"));
        s.remove(QStringLiteral("docking/tab2"));
        s.remove(QStringLiteral("docking/tab3"));
        s.remove(QStringLiteral("docking/bottom"));
        s.sync();
    }

    static QSet<QString> dockNames(const ads::CDockManager* m)
    {
        QSet<QString> names;
        const QMap<QString, ads::CDockWidget*> map = m->dockWidgetsMap();
        for (auto it = map.cbegin(); it != map.cend(); ++it) {
            names.insert(it.key());
        }
        return names;
    }

    static QSplitter* topLevelSplitter(ads::CDockManager* m)
    {
        const auto splitters = m->findChildren<QSplitter*>();
        for (QSplitter* sp : splitters) {
            if (sp->parentWidget() == m) {
                return sp;
            }
        }
        return nullptr;
    }

    // Root splitter of a manager, derived from a dock that belongs to it.
    // Walks up from the dock's area until the direct child of the manager is
    // reached. This is robust against ghost splitters left behind by a
    // restoreState() that replaced the root (deleted via deleteLater()).
    static QSplitter* rootOfDock(ads::CDockManager* m, ads::CDockWidget* d)
    {
        QWidget* w = d->dockAreaWidget();
        while (w && w->parentWidget() != m) {
            w = w->parentWidget();
        }
        return qobject_cast<QSplitter*>(w);
    }

    // Column index of an area inside the manager's top-level splitter:
    // walks up from the dock area until a direct child of the root splitter
    // is reached (the area itself, or a column splitter it lives in).
    static int columnIndexOf(QSplitter* root, QWidget* area)
    {
        QWidget* w = area;
        while (w && w->parentWidget() != root) {
            w = w->parentWidget();
        }
        return w ? root->indexOf(w) : -1;
    }
};

void TestGuiDockLayout::initTestCase()
{
    // Every window in this test reads/writes DABAnalyzerWindow's QSettings
    // store ("StreamDAB-Analyzer","DABAnalyzer"). Redirect it to Qt's test-mode
    // location so the real user config is never touched, and start empty.
    QStandardPaths::setTestModeEnabled(true);
    QSettings(QStringLiteral("StreamDAB-Analyzer"), QStringLiteral("DABAnalyzer")).clear();
}

void TestGuiDockLayout::cleanupTestCase()
{
    QSettings(QStringLiteral("StreamDAB-Analyzer"), QStringLiteral("DABAnalyzer")).clear();
}

void TestGuiDockLayout::testFourDockManagers()
{
    clearSettingsStore();
    DABAnalyzerWindow w;
    w.show();
    QCoreApplication::processEvents();

    const QList<ads::CDockManager*> managers = w.findChildren<ads::CDockManager*>();
    QCOMPARE(managers.size(), 4);

    // Managers are parented appropriately and carry stable objectNames
    QVERIFY(w.findChild<ads::CDockManager*>("tab1DockManager"));
    QVERIFY(w.findChild<ads::CDockManager*>("tab2DockManager"));
    QVERIFY(w.findChild<ads::CDockManager*>("tab3DockManager"));
    QVERIFY(w.findChild<ads::CDockManager*>("bottomDockManager"));

    w.close();
    QCoreApplication::processEvents();
}

void TestGuiDockLayout::test26DocksWithExpectedNames()
{
    clearSettingsStore();
    DABAnalyzerWindow w;
    w.show();
    QCoreApplication::processEvents();

    QSet<QString> all;
    const QList<ads::CDockManager*> managers = w.findChildren<ads::CDockManager*>();
    for (const ads::CDockManager* m : managers) {
        all += dockNames(m);
    }

    const QSet<QString> expected = {
        // Tab 1 - left column (4)
        QStringLiteral("tab1_Player"), QStringLiteral("tab1_Decoder"),
        QStringLiteral("tab1_System"), QStringLiteral("tab1_Messages"),
        // Tab 1 - center column (6)
        QStringLiteral("tab1_InputSource"), QStringLiteral("tab1_EnsembleExplorer"),
        QStringLiteral("tab1_FrameNavigator"), QStringLiteral("tab1_HexCompliance"),
        QStringLiteral("tab1_SubchannelOrg"), QStringLiteral("tab1_NowPlaying"),
        // Tab 1 - right column (6)
        QStringLiteral("tab1_ETIOverview"), QStringLiteral("tab1_ErrorCounter"),
        QStringLiteral("tab1_Status"), QStringLiteral("tab1_Timing"),
        QStringLiteral("tab1_CUUsage"), QStringLiteral("tab1_StreamStats"),
        // Tab 2 (3)
        QStringLiteral("tab2_Left"), QStringLiteral("tab2_Center"),
        QStringLiteral("tab2_Right"),
        // Tab 3 (2)
        QStringLiteral("tab3_Left"), QStringLiteral("tab3_Right"),
        // Shared bottom (5)
        QStringLiteral("bottom_SystemMessages"), QStringLiteral("bottom_Performance"),
        QStringLiteral("bottom_ErrorDetection"), QStringLiteral("bottom_Logging"),
        QStringLiteral("bottom_Constellation"),
    };

    QCOMPARE(all.size(), expected.size());
    QCOMPARE(all, expected);

    w.close();
    QCoreApplication::processEvents();
}

void TestGuiDockLayout::testDefaultColumnPlacement()
{
    clearSettingsStore();
    DABAnalyzerWindow w;
    w.show();
    QCoreApplication::processEvents();

    auto* m1 = w.findChild<ads::CDockManager*>("tab1DockManager");
    QVERIFY(m1);

    QSplitter* root = rootOfDock(m1, m1->findDockWidget("tab1_Player"));
    QVERIFY(root);
    // left column | center column | right column
    QCOMPARE(root->count(), 3);
    // Qt-ADS column splitters are the vertical stacks (first launch).
    for (int i = 0; i < root->count(); ++i) {
        auto* col = qobject_cast<QSplitter*>(root->widget(i));
        QVERIFY2(col && col->orientation() == Qt::Vertical,
                 qPrintable(QString("column %1 must be a vertical stack").arg(i)));
    }

    const QStringList leftNames = {
        QStringLiteral("tab1_Player"), QStringLiteral("tab1_Decoder"),
        QStringLiteral("tab1_System"), QStringLiteral("tab1_Messages")};
    const QStringList centerNames = {
        QStringLiteral("tab1_InputSource"), QStringLiteral("tab1_EnsembleExplorer"),
        QStringLiteral("tab1_FrameNavigator"), QStringLiteral("tab1_HexCompliance"),
        QStringLiteral("tab1_SubchannelOrg"), QStringLiteral("tab1_NowPlaying")};
    const QStringList rightNames = {
        QStringLiteral("tab1_ETIOverview"), QStringLiteral("tab1_ErrorCounter"),
        QStringLiteral("tab1_Status"), QStringLiteral("tab1_Timing"),
        QStringLiteral("tab1_CUUsage"), QStringLiteral("tab1_StreamStats")};

    for (const QString& n : leftNames) {
        ads::CDockWidget* d = m1->findDockWidget(n);
        QVERIFY2(d, qPrintable(n));
        QCOMPARE(columnIndexOf(root, d->dockAreaWidget()), 0);
    }
    for (const QString& n : centerNames) {
        ads::CDockWidget* d = m1->findDockWidget(n);
        QVERIFY2(d, qPrintable(n));
        QCOMPARE(columnIndexOf(root, d->dockAreaWidget()), 1);
    }
    for (const QString& n : rightNames) {
        ads::CDockWidget* d = m1->findDockWidget(n);
        QVERIFY2(d, qPrintable(n));
        QCOMPARE(columnIndexOf(root, d->dockAreaWidget()), 2);
    }

    // Tab 2: Left | Center | Right side-by-side
    auto* m2 = w.findChild<ads::CDockManager*>("tab2DockManager");
    QVERIFY(m2);
    QSplitter* root2 = rootOfDock(m2, m2->findDockWidget("tab2_Left"));
    QVERIFY(root2);
    QCOMPARE(root2->count(), 3);
    QCOMPARE(columnIndexOf(root2, m2->findDockWidget("tab2_Left")->dockAreaWidget()), 0);
    QCOMPARE(columnIndexOf(root2, m2->findDockWidget("tab2_Center")->dockAreaWidget()), 1);
    QCOMPARE(columnIndexOf(root2, m2->findDockWidget("tab2_Right")->dockAreaWidget()), 2);

    // Tab 3: Left | Right side-by-side
    auto* m3 = w.findChild<ads::CDockManager*>("tab3DockManager");
    QVERIFY(m3);
    QSplitter* root3 = rootOfDock(m3, m3->findDockWidget("tab3_Left"));
    QVERIFY(root3);
    QCOMPARE(root3->count(), 2);
    QCOMPARE(columnIndexOf(root3, m3->findDockWidget("tab3_Left")->dockAreaWidget()), 0);
    QCOMPARE(columnIndexOf(root3, m3->findDockWidget("tab3_Right")->dockAreaWidget()), 1);

    w.close();
    QCoreApplication::processEvents();
}

void TestGuiDockLayout::testBottomFiveDocks()
{
    clearSettingsStore();
    DABAnalyzerWindow w;
    w.show();
    QCoreApplication::processEvents();

    auto* mBottom = w.findChild<ads::CDockManager*>("bottomDockManager");
    QVERIFY(mBottom);

    const QSet<QString> bottomNames = {
        QStringLiteral("bottom_SystemMessages"), QStringLiteral("bottom_Performance"),
        QStringLiteral("bottom_ErrorDetection"), QStringLiteral("bottom_Logging"),
        QStringLiteral("bottom_Constellation")};
    const QSet<QString> actual = dockNames(mBottom);
    QCOMPARE(actual, bottomNames);
    QCOMPARE(actual.size(), 5);

    // The 5 bottom docks are TABBED into a single shared dock area. Note: a
    // restoreState() may leave a ghost area pending deleteLater, so derive the
    // areas from the docks themselves instead of findChildren().
    ads::CDockAreaWidget* sharedArea = nullptr;
    for (const QString& n : bottomNames) {
        ads::CDockWidget* d = mBottom->findDockWidget(n);
        QVERIFY2(d, qPrintable(n));
        if (!sharedArea) {
            sharedArea = d->dockAreaWidget();
            QVERIFY(sharedArea);
        } else {
            QCOMPARE(d->dockAreaWidget(), sharedArea);
        }
    }
    QVERIFY(sharedArea);
    QCOMPARE(sharedArea->dockWidgetsCount(), 5);

    w.close();
    QCoreApplication::processEvents();
}

void TestGuiDockLayout::testPersistenceRoundTrip()
{
    // Isolate QSettings in Qt's test-mode writable location so the real user
    // config is never touched, and start from a cleared store.
    QStandardPaths::setTestModeEnabled(true);
    clearSettingsStore();

    const QString hiddenDockName = QStringLiteral("tab1_NowPlaying");

    // --- First launch: default layout, hide one dock, close (saves state) ---
    {
        DABAnalyzerWindow w;
        w.show();
        QCoreApplication::processEvents();

        auto* m1 = w.findChild<ads::CDockManager*>("tab1DockManager");
        QVERIFY(m1);
        ads::CDockWidget* dock = m1->findDockWidget(hiddenDockName);
        QVERIFY(dock);
        QVERIFY(!dock->isClosed());

        dock->toggleView(false);
        QVERIFY(dock->isClosed());
        QCoreApplication::processEvents();

        w.close(); // triggers closeEvent -> saveDockingState()
        QCoreApplication::processEvents();
    }

    const QSettings check(QStringLiteral("StreamDAB-Analyzer"), QStringLiteral("DABAnalyzer"));
    QVERIFY(!check.value(QStringLiteral("docking/tab1")).isNull());
    QVERIFY(!check.value(QStringLiteral("docking/tab2")).isNull());
    QVERIFY(!check.value(QStringLiteral("docking/tab3")).isNull());
    QVERIFY(!check.value(QStringLiteral("docking/bottom")).isNull());

    // --- Second launch: restored state must keep the dock hidden ---
    {
        DABAnalyzerWindow w2;
        w2.show();
        QCoreApplication::processEvents();

        auto* m1 = w2.findChild<ads::CDockManager*>("tab1DockManager");
        QVERIFY(m1);
        ads::CDockWidget* restored = m1->findDockWidget(hiddenDockName);
        QVERIFY(restored);
        QVERIFY2(restored->isClosed() || !restored->isVisible(),
                 qPrintable("hidden dock must stay hidden after state restore: "
                            + hiddenDockName));

        // Sanity: the other docks are restored (present, not closed).
        ads::CDockWidget* player = m1->findDockWidget(QStringLiteral("tab1_Player"));
        QVERIFY(player);
        QVERIFY(!player->isClosed());

        // Restore must preserve the COLUMN structure, not just visibility:
        // root splitter count + orientation, and each dock's column index,
        // must match the default first-launch layout (same assertions as
        // testDefaultColumnPlacement).
        QSplitter* root = rootOfDock(m1, player);
        QVERIFY(root);
        // left column | center column | right column
        QCOMPARE(root->count(), 3);
        for (int i = 0; i < root->count(); ++i) {
            auto* col = qobject_cast<QSplitter*>(root->widget(i));
            QVERIFY2(col && col->orientation() == Qt::Vertical,
                     qPrintable(QString("restored column %1 must be a vertical stack").arg(i)));
        }

        const QStringList leftNames = {
            QStringLiteral("tab1_Player"), QStringLiteral("tab1_Decoder"),
            QStringLiteral("tab1_System"), QStringLiteral("tab1_Messages")};
        const QStringList centerNames = {
            QStringLiteral("tab1_InputSource"), QStringLiteral("tab1_EnsembleExplorer"),
            QStringLiteral("tab1_FrameNavigator"), QStringLiteral("tab1_HexCompliance"),
            QStringLiteral("tab1_SubchannelOrg"), QStringLiteral("tab1_NowPlaying")};
        const QStringList rightNames = {
            QStringLiteral("tab1_ETIOverview"), QStringLiteral("tab1_ErrorCounter"),
            QStringLiteral("tab1_Status"), QStringLiteral("tab1_Timing"),
            QStringLiteral("tab1_CUUsage"), QStringLiteral("tab1_StreamStats")};

        for (const QString& n : leftNames) {
            ads::CDockWidget* d = m1->findDockWidget(n);
            QVERIFY2(d, qPrintable(n));
            QCOMPARE(columnIndexOf(root, d->dockAreaWidget()), 0);
        }
        for (const QString& n : centerNames) {
            ads::CDockWidget* d = m1->findDockWidget(n);
            QVERIFY2(d, qPrintable(n));
            if (d->isClosed()) {
                // Hidden dock: closed-ness already asserted above; still verify
                // its restored column when the area is attached.
                if (ads::CDockAreaWidget* a = d->dockAreaWidget()) {
                    QCOMPARE(columnIndexOf(root, a), 1);
                }
                continue;
            }
            QCOMPARE(columnIndexOf(root, d->dockAreaWidget()), 1);
        }
        for (const QString& n : rightNames) {
            ads::CDockWidget* d = m1->findDockWidget(n);
            QVERIFY2(d, qPrintable(n));
            QCOMPARE(columnIndexOf(root, d->dockAreaWidget()), 2);
        }

        w2.close();
        QCoreApplication::processEvents();
    }

    // Clean up the test-mode store so no state leaks into other test runs.
    clearSettingsStore();
}

void TestGuiDockLayout::testFloatingRoundTrip()
{
    clearSettingsStore();
    DABAnalyzerWindow w;
    w.show();
    QCoreApplication::processEvents();

    auto* m1 = w.findChild<ads::CDockManager*>("tab1DockManager");
    QVERIFY(m1);
    ads::CDockWidget* dock = m1->findDockWidget(QStringLiteral("tab1_InputSource"));
    QVERIFY(dock);
    QVERIFY(dock->dockAreaWidget());
    QVERIFY(!dock->isFloating());

    // Detach into a floating window (ADS 4.5.0: setFloating() takes no arg
    // and floats the whole dock area; isFloating() is only true when the dock
    // is the one-and-only widget in the floating container, so accept
    // isInFloatingContainer() too).
    dock->setFloating();
    QCoreApplication::processEvents();
    QVERIFY2(dock->isFloating() || dock->isInFloatingContainer(),
             "dock must be floating after setFloating()");

    // Re-dock it into the manager: the dock must be managed/present and
    // visible again, and the process must not crash (proxy for post-root-
    // surgery drag/float safety).
    m1->addDockWidget(ads::TopDockWidgetArea, dock, nullptr);
    QCoreApplication::processEvents();
    QVERIFY(!dock->isInFloatingContainer());
    QVERIFY(!dock->isClosed());
    QVERIFY(dock->isVisible());
    QVERIFY(dock->dockAreaWidget());

    w.close();
    QCoreApplication::processEvents();
}

void TestGuiDockLayout::testAutoHideConfig()
{
    // Auto-hide (pin) config was enabled once in setupUI() before the first
    // CDockManager was constructed. 4.5.0 exposes both getters.
    const ads::CDockManager::AutoHideFlags flags = ads::CDockManager::autoHideConfigFlags();
    QVERIFY2(flags.testFlag(ads::CDockManager::AutoHideFeatureEnabled),
             "auto-hide / pin feature must stay enabled");
    QVERIFY(ads::CDockManager::testAutoHideConfigFlag(ads::CDockManager::AutoHideFeatureEnabled));
    // Default dock features (float/close/move/pin) must NOT be disabled.
    const ads::CDockWidget::DockWidgetFeatures defaults =
        ads::CDockWidget::DefaultDockWidgetFeatures;
    QVERIFY(defaults.testFlag(ads::CDockWidget::DockWidgetClosable));
    QVERIFY(defaults.testFlag(ads::CDockWidget::DockWidgetFloatable));
    QVERIFY(defaults.testFlag(ads::CDockWidget::DockWidgetMovable));
    QVERIFY(defaults.testFlag(ads::CDockWidget::DockWidgetPinnable));
}

QTEST_MAIN(TestGuiDockLayout)
#include "test_gui_dock_layout.moc"