/**
 * @file test_gui_dock_layout.cpp
 * @brief Qt-ADS (Qt Advanced Docking System) layout tests for DABAnalyzerWindow
 *
 * UI redesign (v2) validation:
 *   a) exactly 4 CDockManagers (one per tab + one window-level bottom dock area)
 *   b) exactly 25 dock widgets with the expected objectNames
 *      (14 tab1 + 3 tab2 + 2 tab3 + 6 bottom; -tab1_InputSource -> toolbar,
 *       -tab1_Messages -> bottom_Messages strip tab)
 *   c) default first-launch placement: TABBED GROUPS on Tab 1 —
 *      Transport (Player|Decoder) / System left; Signal Analysis
 *      (Ensemble|Subchannel|CUUsage) / Frame Analysis (Navigator|HexCompliance) /
 *      Now Playing center; Signal Info (Overview|Status|Timing) /
 *      Error&Stats (ErrorCounter|StreamStats) right; 6 tabbed docks in the
 *      bottom manager
 *   d) the Input Source toolbar sits at the TOP of the Tab-1 page with an
 *      expandable network row (visible only in Network Stream mode)
 *   e) QSettings persistence round-trip (closed dock stays closed; restored
 *      group structure matches the default layout) incl. layout_version=2
 *   f) stale v1 layout state (layout_version != 2) is discarded once so the
 *      first run after the upgrade shows the new default layout
 *   g) auto-hide (pin) config sanity
 *   h) floating round-trip safety (float a dock, re-dock it, no crash)
 *   i) default ACTIVE tab per tabbed group on a fresh launch (first dock of
 *      each group: Player, Ensemble Explorer, Frame Navigator, ETI Overview,
 *      DAB Error Counter, System Messages for the bottom strip)
 *   j) PANEL_AUDIT real-data wiring: after loading the real Bangkok fixture
 *      the panels show DECODED data — ensemble tree ("Bangkok DAB+", "RROne
 *      FM 101"), 16-row EEP subchannel table, Tab-2 overview labels with real
 *      ensemble/FIG/subchannel counts, Tab-2 service table with labels,
 *      Tab-3 FIG instance tracer (FIG 0/0 + 1/0 rows + real element tree),
 *      and the FIB CRC error counter at 0 on the clean capture.
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
#include <QStackedWidget>
#include <QRadioButton>
#include <QPushButton>
#include <QLineEdit>
#include <QVBoxLayout>
#include <functional>

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
    void test25DocksWithExpectedNames();
    void testDefaultGroupedPlacement();
    void testDefaultActiveTabs();
    void testBottomSixDocks();
    void testInputToolbar();
    void testPersistenceRoundTrip();
    void testLayoutVersionReset();
    void testFloatingRoundTrip();
    void testAutoHideConfig();
    void testReloadResetsAnalyzerState();
    void testEnsembleTreeRealFixture();

private:
    // Removes every DABAnalyzerWindow QSettings key used by the app so each
    // window-based test starts from a CLEARED store. window.close() saves dock
    // state; without this, the next window would restore the previous layout
    // and break first-launch expectations.
    static void clearSettingsStore()
    {
        QSettings s(QStringLiteral("StreamDAB-Analyzer"), QStringLiteral("DABAnalyzer"));
        s.remove(QStringLiteral("window/geometry"));
        s.remove(QStringLiteral("docking/layout_version"));
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

    // UI-redesign GROUP verification: every named dock must (a) exist,
    // (b) live in the same MENBER dock area (tabified group), (c) the area
    // must hold exactly areaDockCount tabs, (d) the area must sit in the
    // given column of the root splitter.
    static void verifyTabbedGroup(ads::CDockManager* m, QSplitter* root,
                                  const QStringList& names,
                                  int areaDockCount, int column)
    {
        ads::CDockAreaWidget* shared = nullptr;
        for (const QString& n : names) {
            ads::CDockWidget* d = m->findDockWidget(n);
            QVERIFY2(d, qPrintable(n));
            ads::CDockAreaWidget* a = d->dockAreaWidget();
            QVERIFY2(a, qPrintable(QString("%1 must have a dock area").arg(n)));
            if (!shared) {
                shared = a;
            } else {
                QCOMPARE(a, shared);
            }
            QCOMPARE(columnIndexOf(root, a), column);
        }
        QVERIFY(shared);
        QCOMPARE(shared->dockWidgetsCount(), areaDockCount);
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

void TestGuiDockLayout::test25DocksWithExpectedNames()
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
        // Tab 1 (14) - UI redesign: tab1_InputSource removed (toolbar),
        // tab1_Messages removed (bottom strip), CU Usage grouped in center.
        QStringLiteral("tab1_Player"), QStringLiteral("tab1_Decoder"),
        QStringLiteral("tab1_System"),
        QStringLiteral("tab1_EnsembleExplorer"), QStringLiteral("tab1_SubchannelOrg"),
        QStringLiteral("tab1_CUUsage"),
        QStringLiteral("tab1_FrameNavigator"), QStringLiteral("tab1_HexCompliance"),
        QStringLiteral("tab1_NowPlaying"),
        QStringLiteral("tab1_ETIOverview"), QStringLiteral("tab1_Status"),
        QStringLiteral("tab1_Timing"),
        QStringLiteral("tab1_ErrorCounter"), QStringLiteral("tab1_StreamStats"),
        // Tab 2 (3)
        QStringLiteral("tab2_Left"), QStringLiteral("tab2_Center"),
        QStringLiteral("tab2_Right"),
        // Tab 3 (2)
        QStringLiteral("tab3_Left"), QStringLiteral("tab3_Right"),
        // Shared bottom (6) - incl. bottom_Messages (moved from Tab 1)
        QStringLiteral("bottom_SystemMessages"), QStringLiteral("bottom_Performance"),
        QStringLiteral("bottom_ErrorDetection"), QStringLiteral("bottom_Logging"),
        QStringLiteral("bottom_Constellation"), QStringLiteral("bottom_Messages"),
    };

    QCOMPARE(all.size(), expected.size());  // 25 docks
    QCOMPARE(all, expected);

    w.close();
    QCoreApplication::processEvents();
}

void TestGuiDockLayout::testDefaultGroupedPlacement()
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

    // Removed docks: Input Source is the toolbar now; Messages sits in the
    // bottom strip.
    QVERIFY(!m1->findDockWidget(QStringLiteral("tab1_InputSource")));
    QVERIFY(!m1->findDockWidget(QStringLiteral("tab1_Messages")));

    // LEFT column (0): Transport group (Player | Decoder) + System standalone.
    verifyTabbedGroup(m1, root,
                      {QStringLiteral("tab1_Player"), QStringLiteral("tab1_Decoder")},
                      2, 0);
    ads::CDockAreaWidget* transportArea =
        m1->findDockWidget(QStringLiteral("tab1_Player"))->dockAreaWidget();
    QVERIFY(transportArea);
    ads::CDockAreaWidget* systemArea =
        m1->findDockWidget(QStringLiteral("tab1_System"))->dockAreaWidget();
    QVERIFY(systemArea);
    QCOMPARE(systemArea->dockWidgetsCount(), 1);
    QCOMPARE(columnIndexOf(root, systemArea), 0);
    QVERIFY(systemArea != transportArea);

    // CENTER column (1): Signal Analysis (3 tabs), Frame Analysis (2 tabs),
    // Now Playing standalone (compact, bottom of center).
    verifyTabbedGroup(m1, root,
                      {QStringLiteral("tab1_EnsembleExplorer"),
                       QStringLiteral("tab1_SubchannelOrg"),
                       QStringLiteral("tab1_CUUsage")},
                      3, 1);
    ads::CDockAreaWidget* signalAnalysisArea =
        m1->findDockWidget(QStringLiteral("tab1_EnsembleExplorer"))->dockAreaWidget();
    QVERIFY(signalAnalysisArea);
    verifyTabbedGroup(m1, root,
                      {QStringLiteral("tab1_FrameNavigator"),
                       QStringLiteral("tab1_HexCompliance")},
                      2, 1);
    QVERIFY(m1->findDockWidget(QStringLiteral("tab1_FrameNavigator"))->dockAreaWidget()
            != signalAnalysisArea);
    ads::CDockAreaWidget* nowPlayingArea =
        m1->findDockWidget(QStringLiteral("tab1_NowPlaying"))->dockAreaWidget();
    QVERIFY(nowPlayingArea);
    QCOMPARE(nowPlayingArea->dockWidgetsCount(), 1);
    QCOMPARE(columnIndexOf(root, nowPlayingArea), 1);
    QVERIFY(nowPlayingArea != signalAnalysisArea);

    // RIGHT column (2): Signal Info (3 tabs) + Error & Statistics (2 tabs).
    verifyTabbedGroup(m1, root,
                      {QStringLiteral("tab1_ETIOverview"),
                       QStringLiteral("tab1_Status"),
                       QStringLiteral("tab1_Timing")},
                      3, 2);
    ads::CDockAreaWidget* signalInfoArea =
        m1->findDockWidget(QStringLiteral("tab1_ETIOverview"))->dockAreaWidget();
    QVERIFY(signalInfoArea);
    verifyTabbedGroup(m1, root,
                      {QStringLiteral("tab1_ErrorCounter"),
                       QStringLiteral("tab1_StreamStats")},
                      2, 2);
    QVERIFY(m1->findDockWidget(QStringLiteral("tab1_ErrorCounter"))->dockAreaWidget()
            != signalInfoArea);

    // Tab 2: Left | Center | Right side-by-side (unchanged by the redesign).
    auto* m2 = w.findChild<ads::CDockManager*>("tab2DockManager");
    QVERIFY(m2);
    QSplitter* root2 = rootOfDock(m2, m2->findDockWidget("tab2_Left"));
    QVERIFY(root2);
    QCOMPARE(root2->count(), 3);
    QCOMPARE(columnIndexOf(root2, m2->findDockWidget("tab2_Left")->dockAreaWidget()), 0);
    QCOMPARE(columnIndexOf(root2, m2->findDockWidget("tab2_Center")->dockAreaWidget()), 1);
    QCOMPARE(columnIndexOf(root2, m2->findDockWidget("tab2_Right")->dockAreaWidget()), 2);

    // Tab 3: Left | Right side-by-side (unchanged by the redesign).
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

void TestGuiDockLayout::testDefaultActiveTabs()
{
    clearSettingsStore();
    DABAnalyzerWindow w;
    w.show();
    QCoreApplication::processEvents();

    auto* m1 = w.findChild<ads::CDockManager*>("tab1DockManager");
    QVERIFY(m1);

    // On a fresh (version-reset) window every tabbed group must show its
    // FIRST dock as the active tab. Tab ORDER is unchanged; only the
    // visible/active tab differs.
    auto verifyGroupActive = [m1](const QString& groupFirstDock, const QString& expectedActive) {
        ads::CDockWidget* first = m1->findDockWidget(groupFirstDock);
        QVERIFY2(first, qPrintable(groupFirstDock));
        ads::CDockAreaWidget* area = first->dockAreaWidget();
        QVERIFY2(area, qPrintable(QString("%1 must have a dock area").arg(groupFirstDock)));
        ads::CDockWidget* current = area->currentDockWidget();
        QVERIFY2(current, qPrintable(QString("%1 area must have a current dock").arg(groupFirstDock)));
        QCOMPARE(current->objectName(), expectedActive);
    };

    // Transport group
    verifyGroupActive(QStringLiteral("tab1_Player"), QStringLiteral("tab1_Player"));
    // Signal Analysis group
    verifyGroupActive(QStringLiteral("tab1_EnsembleExplorer"), QStringLiteral("tab1_EnsembleExplorer"));
    // Frame Analysis group
    verifyGroupActive(QStringLiteral("tab1_FrameNavigator"), QStringLiteral("tab1_FrameNavigator"));
    // Signal Info group
    verifyGroupActive(QStringLiteral("tab1_ETIOverview"), QStringLiteral("tab1_ETIOverview"));
    // Error & Statistics group
    verifyGroupActive(QStringLiteral("tab1_ErrorCounter"), QStringLiteral("tab1_ErrorCounter"));

    // Bottom strip: 6 tabbed docks with System Messages active on launch.
    auto* mBottom = w.findChild<ads::CDockManager*>("bottomDockManager");
    QVERIFY(mBottom);
    ads::CDockAreaWidget* bottomArea =
        mBottom->findDockWidget(QStringLiteral("bottom_SystemMessages"))->dockAreaWidget();
    QVERIFY(bottomArea);
    QCOMPARE(bottomArea->dockWidgetsCount(), 6);
    ads::CDockWidget* bottomCurrent = bottomArea->currentDockWidget();
    QVERIFY(bottomCurrent);
    QCOMPARE(bottomCurrent->objectName(), QStringLiteral("bottom_SystemMessages"));

    w.close();
    QCoreApplication::processEvents();
}

void TestGuiDockLayout::testBottomSixDocks()
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
        QStringLiteral("bottom_Constellation"), QStringLiteral("bottom_Messages")};
    const QSet<QString> actual = dockNames(mBottom);
    QCOMPARE(actual, bottomNames);
    QCOMPARE(actual.size(), 6);

    // The 6 bottom docks are TABBED into a single shared dock area. Note: a
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
    QCOMPARE(sharedArea->dockWidgetsCount(), 6);

    w.close();
    QCoreApplication::processEvents();
}

void TestGuiDockLayout::testInputToolbar()
{
    clearSettingsStore();
    DABAnalyzerWindow w;
    w.show();
    QCoreApplication::processEvents();

    // The Input Source toolbar is a child of the Tab-1 page.
    QWidget* toolbar = w.findChild<QWidget*>(QStringLiteral("inputToolbar"));
    QVERIFY(toolbar);
    QVERIFY(toolbar->isVisible());

    auto* m1 = w.findChild<ads::CDockManager*>("tab1DockManager");
    QVERIFY(m1);
    // Same page, ABOVE the page's CDockManager in the page QVBoxLayout.
    QCOMPARE(toolbar->parentWidget(), m1->parentWidget());
    auto* pageLayout = qobject_cast<QVBoxLayout*>(toolbar->parentWidget()->layout());
    QVERIFY(pageLayout);
    QCOMPARE(pageLayout->itemAt(0)->widget(), toolbar);
    QCOMPARE(pageLayout->itemAt(1)->widget(), m1);

    // Row 1 controls: File/Network radios, path edit, Browse..., Load File.
    QRadioButton* fileRadio = nullptr;
    QRadioButton* netRadio = nullptr;
    const QList<QRadioButton*> radios = toolbar->findChildren<QRadioButton*>();
    for (QRadioButton* r : radios) {
        if (r->text() == QLatin1String("File")) fileRadio = r;
        else if (r->text() == QLatin1String("Network Stream")) netRadio = r;
    }
    QVERIFY(fileRadio);
    QVERIFY(netRadio);
    const QList<QLineEdit*> lineEdits = toolbar->findChildren<QLineEdit*>();
    QVERIFY(lineEdits.size() >= 2);  // file path + stream URL
    bool hasBrowse = false, hasLoad = false;
    const QList<QPushButton*> buttons = toolbar->findChildren<QPushButton*>();
    for (const QPushButton* b : buttons) {
        if (b->text() == QLatin1String("Browse...")) hasBrowse = true;
        if (b->text() == QLatin1String("Load File")) hasLoad = true;
    }
    QVERIFY(hasBrowse);
    QVERIFY(hasLoad);

    // Network row: hidden by default (File mode), expands on Network radio.
    auto* netRow = toolbar->findChild<QStackedWidget*>(QStringLiteral("inputToolbarNetworkRow"));
    QVERIFY(netRow);
    QVERIFY(!netRow->isVisible());
    QVERIFY(fileRadio->isChecked());

    netRadio->setChecked(true);
    QCoreApplication::processEvents();
    QVERIFY(netRow->isVisible());
    QCOMPARE(netRow->currentIndex(), 1);
    QVERIFY(!fileRadio->isChecked());
    // Network controls are inside the expanded row.
    QVERIFY(netRow->findChild<QComboBox*>());  // quick examples combo

    fileRadio->setChecked(true);
    QCoreApplication::processEvents();
    QVERIFY(!netRow->isVisible());
    QCOMPARE(netRow->currentIndex(), 0);
    QVERIFY(!netRadio->isChecked());

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
    QCOMPARE(check.value(QStringLiteral("docking/layout_version")).toInt(), 2);
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

        // Restore must preserve the GROUP structure, not just visibility:
        // root splitter count + orientation, and each group's area/column,
        // must match the default first-launch layout (same assertions as
        // testDefaultGroupedPlacement).
        ads::CDockWidget* player = m1->findDockWidget(QStringLiteral("tab1_Player"));
        QVERIFY(player);
        QVERIFY(!player->isClosed());
        QSplitter* root = rootOfDock(m1, player);
        QVERIFY(root);
        QCOMPARE(root->count(), 3);
        for (int i = 0; i < root->count(); ++i) {
            auto* col = qobject_cast<QSplitter*>(root->widget(i));
            QVERIFY2(col && col->orientation() == Qt::Vertical,
                     qPrintable(QString("restored column %1 must be a vertical stack").arg(i)));
        }

        // Group verification (all groups fully visible; only NowPlaying closed).
        verifyTabbedGroup(m1, root,
                          {QStringLiteral("tab1_Player"), QStringLiteral("tab1_Decoder")},
                          2, 0);
        QCOMPARE(columnIndexOf(root,
                 m1->findDockWidget(QStringLiteral("tab1_System"))->dockAreaWidget()), 0);
        verifyTabbedGroup(m1, root,
                          {QStringLiteral("tab1_EnsembleExplorer"),
                           QStringLiteral("tab1_SubchannelOrg"),
                           QStringLiteral("tab1_CUUsage")},
                          3, 1);
        verifyTabbedGroup(m1, root,
                          {QStringLiteral("tab1_FrameNavigator"),
                           QStringLiteral("tab1_HexCompliance")},
                          2, 1);
        QCOMPARE(columnIndexOf(root,
                 m1->findDockWidget(QStringLiteral("tab1_ErrorCounter"))->dockAreaWidget()), 2);
        verifyTabbedGroup(m1, root,
                          {QStringLiteral("tab1_ETIOverview"),
                           QStringLiteral("tab1_Status"),
                           QStringLiteral("tab1_Timing")},
                          3, 2);
        verifyTabbedGroup(m1, root,
                          {QStringLiteral("tab1_ErrorCounter"),
                           QStringLiteral("tab1_StreamStats")},
                          2, 2);

        // Hidden dock: closed-ness already asserted above; still verify its
        // restored column when the area is attached.
        if (ads::CDockAreaWidget* a = restored->dockAreaWidget()) {
            QCOMPARE(columnIndexOf(root, a), 1);
        }

        w2.close();
        QCoreApplication::processEvents();
    }

    // Clean up the test-mode store so no state leaks into other test runs.
    clearSettingsStore();
}

void TestGuiDockLayout::testLayoutVersionReset()
{
    // Stale v1 settings (layout_version 1 + a saved layout with Now Playing
    // closed) must be DISCARDED once: the next launch shows the new default
    // layout (Now Playing visible, groups intact) and re-saves version 2.
    QStandardPaths::setTestModeEnabled(true);
    clearSettingsStore();

    const QString hiddenDockName = QStringLiteral("tab1_NowPlaying");
    const QString versionKey = QStringLiteral("docking/layout_version");

    // 1) First launch: hide Now Playing, close -> saves version-2 state.
    {
        DABAnalyzerWindow w;
        w.show();
        QCoreApplication::processEvents();

        auto* m1 = w.findChild<ads::CDockManager*>("tab1DockManager");
        QVERIFY(m1);
        ads::CDockWidget* np = m1->findDockWidget(hiddenDockName);
        QVERIFY(np);
        np->toggleView(false);
        QCoreApplication::processEvents();

        w.close(); // triggers closeEvent -> saveDockingState()
        QCoreApplication::processEvents();
    }
    {
        const QSettings check(QStringLiteral("StreamDAB-Analyzer"), QStringLiteral("DABAnalyzer"));
        QCOMPARE(check.value(versionKey).toInt(), 2);
        QVERIFY(!check.value(QStringLiteral("docking/tab1")).isNull());
    }

    // 2) Simulate a stale v1 store: downgrade the layout version.
    {
        QSettings s(QStringLiteral("StreamDAB-Analyzer"), QStringLiteral("DABAnalyzer"));
        s.setValue(versionKey, 1);
        s.sync();
    }

    // 3) Second launch: stale state discarded -> default redesign layout.
    {
        DABAnalyzerWindow w2;
        w2.show();
        QCoreApplication::processEvents();

        auto* m1 = w2.findChild<ads::CDockManager*>("tab1DockManager");
        QVERIFY(m1);
        ads::CDockWidget* np = m1->findDockWidget(hiddenDockName);
        QVERIFY(np);
        QVERIFY2(!np->isClosed() && np->isVisible(),
                 "stale v1 layout must be discarded: Now Playing visible "
                 "in the default redesign layout");

        // Default group structure present (same as first launch).
        QSplitter* root = rootOfDock(m1, m1->findDockWidget(QStringLiteral("tab1_Player")));
        QVERIFY(root);
        QCOMPARE(root->count(), 3);
        verifyTabbedGroup(m1, root,
                          {QStringLiteral("tab1_Player"), QStringLiteral("tab1_Decoder")},
                          2, 0);
        verifyTabbedGroup(m1, root,
                          {QStringLiteral("tab1_EnsembleExplorer"),
                           QStringLiteral("tab1_SubchannelOrg"),
                           QStringLiteral("tab1_CUUsage")},
                          3, 1);
        verifyTabbedGroup(m1, root,
                          {QStringLiteral("tab1_ETIOverview"),
                           QStringLiteral("tab1_Status"),
                           QStringLiteral("tab1_Timing")},
                          3, 2);

        auto* mBottom = w2.findChild<ads::CDockManager*>("bottomDockManager");
        QVERIFY(mBottom);
        QVERIFY(mBottom->findDockWidget(QStringLiteral("bottom_Messages")));

        w2.close(); // writes layout version 2 again
        QCoreApplication::processEvents();
    }

    // 4) Version is 2 again; a third launch restores normally (v2 round-trip).
    {
        const QSettings s(QStringLiteral("StreamDAB-Analyzer"), QStringLiteral("DABAnalyzer"));
        QCOMPARE(s.value(versionKey).toInt(), 2);
    }
    {
        DABAnalyzerWindow w3;
        w3.show();
        QCoreApplication::processEvents();

        auto* m1 = w3.findChild<ads::CDockManager*>("tab1DockManager");
        QVERIFY(m1);
        ads::CDockWidget* np = m1->findDockWidget(hiddenDockName);
        QVERIFY(np);
        QVERIFY(!np->isClosed());  // v2 state restored (Now Playing visible)

        w3.close();
        QCoreApplication::processEvents();
    }

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
    // A dock inside a TABBED group: floating must stay safe and re-docking
    // must not crash (post-root-surgery drag/float safety).
    ads::CDockWidget* dock = m1->findDockWidget(QStringLiteral("tab1_CUUsage"));
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
    // visible again, and the process must not crash.
    m1->addDockWidget(ads::TopDockWidgetArea, dock, nullptr);
    QCoreApplication::processEvents();
    QVERIFY(!dock->isInFloatingContainer());
    QVERIFY(!dock->isClosed());
    QVERIFY(dock->isVisible());
    QVERIFY(dock->dockAreaWidget());

    w.close();
    QCoreApplication::processEvents();
}

void TestGuiDockLayout::testReloadResetsAnalyzerState()
{
    // Finding 10: loadETIFile() must reset the FIG analyzer before a NEW
    // file so Tab-3 instances / FIG counters / FIB-CRC counts restart clean
    // instead of accumulating across files.
    clearSettingsStore();
    DABAnalyzerWindow w;
    w.show();
    QCoreApplication::processEvents();

    // cwd for ctest GUI tests is <build>/tests -> fixture lives two levels up
    const QString fixture = QStringLiteral("../../eti/bkk_20062022_141637.eti");
    QVERIFY2(QFileInfo::exists(fixture),
             qPrintable(QString("fixture missing: %1 (cwd=%2)")
                            .arg(fixture, QDir::currentPath())));

    // --- First load: full fixture ---
    w.autoLoadFile(fixture);
    QTRY_VERIFY_WITH_TIMEOUT(w.isFileLoaded() && w.analyzerFigInstanceCount() > 0,
                             120000);
    const int inst1 = w.analyzerFigInstanceCount();
    const int total1 = w.analyzerTotalFigCount();
    QVERIFY2(total1 > 0, "first load must decode FIGs");
    QVERIFY2(inst1 > 0, "first load must record FIG instances");

    // --- Second load of the same file: without the analyzer reset the
    // instance list / counters would accumulate (inst2 ~= inst1 + inst1) ---
    w.autoLoadFile(fixture);
    QTRY_VERIFY_WITH_TIMEOUT(w.isFileLoaded() && w.analyzerFigInstanceCount() > 0,
                             120000);
    const int inst2 = w.analyzerFigInstanceCount();
    const int total2 = w.analyzerTotalFigCount();
    QVERIFY2(inst2 <= inst1,
             qPrintable(QString("FIG instances accumulated across reloads: "
                                "%1 -> %2").arg(inst1).arg(inst2)));
    QVERIFY2(total2 <= total1,
             qPrintable(QString("FIG counters accumulated across reloads: "
                                "%1 -> %2").arg(total1).arg(total2)));
    // FIB-CRC failures stay at 0 on the clean capture across reloads.
    QCOMPARE(w.analyzerFibCrcFailureCount(), 0);
    // The Tab-3 instance tree reflects only the second load.
    QCOMPARE(w.tab3FigInstanceTree()->topLevelItemCount(), inst2);

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

void TestGuiDockLayout::testEnsembleTreeRealFixture()
{
    // GUI decode-path validation on the REAL Bangkok fixture: after the file
    // loads and the frame/FIG processing pipeline drains, the DAB Ensemble
    // Explorer tree must show the decoded ensemble label ("Bangkok DAB+",
    // EId 0x2000) and the decoded service label ("RROne FM 101") — not
    // mojibake/garbage from the pre-fix FIG walking (this test documents the
    // v1.3 GUI decode fix; the deterministic decode itself is covered by
    // test_advanced_fig_analyzer).
    clearSettingsStore();
    DABAnalyzerWindow w;
    w.show();
    QCoreApplication::processEvents();

    // cwd for ctest GUI tests is <build>/tests -> fixture lives two levels up
    const QString fixture = QStringLiteral("../../eti/bkk_20062022_141637.eti");
    QVERIFY2(QFileInfo::exists(fixture),
             qPrintable(QString("fixture missing: %1 (cwd=%2)")
                            .arg(fixture, QDir::currentPath())));
    w.autoLoadFile(fixture);

    // Frame processing is synchronous inside processETIFile(); drain any
    // queued signals, then wait (with the event loop) for the tree to be
    // populated by onProcessingComplete() -> updateEnsembleTree().
    QCoreApplication::processEvents();
    QTRY_VERIFY_WITH_TIMEOUT(w.ensembleTree() &&
                                 w.ensembleTree()->topLevelItemCount() > 0,
                             120000);

    // The top-level item is the ensemble: "Bangkok DAB+ [EID: 0x2000]"
    QTreeWidgetItem* ensembleItem = w.ensembleTree()->topLevelItem(0);
    QVERIFY(ensembleItem);
    const QString ensembleText = ensembleItem->text(0);
    QVERIFY2(ensembleText.contains(QStringLiteral("Bangkok DAB+"),
                                   Qt::CaseInsensitive),
             qPrintable(QString("ensemble tree text: %1").arg(ensembleText)));

    // Services are children of the ensemble item; the fixture's service
    // 0x2C61 must appear with its decoded label (ASCII charset 0; the app
    // upper-cases display text).
    bool foundRROne = false;
    for (int i = 0; i < ensembleItem->childCount(); ++i) {
        const QString svcText = ensembleItem->child(i)->text(0);
        if (svcText.contains(QStringLiteral("RROne FM 101"), Qt::CaseInsensitive)) {
            foundRROne = true;
            QVERIFY2(!svcText.contains(QStringLiteral("\uFFFD")),
                     "service label must not contain replacement characters");
            break;
        }
    }
    QVERIFY2(foundRROne, "service 'RROne FM 101' not found in ensemble tree");

    // =====================================================================
    // PANEL_AUDIT real-data assertions (v1.3 GUI wiring wave): every panel
    // below must show DECODED data after the load, never the old
    // "Ready"/"Available"/"TODO" literals or simulated content.
    // =====================================================================

    // --- Subchannel Organization (Tab 1 center): 16 real subchannels,
    // EEP (etisnoop-parity semantics: EEP = long form) ---
    QTableWidget* subT = w.subchannelTable();
    QVERIFY(subT);
    QCOMPARE(subT->rowCount(), 16);
    bool foundEep = false;
    for (int r = 0; r < subT->rowCount() && !foundEep; ++r) {
        for (int c = 0; c < subT->columnCount(); ++c) {
            QTableWidgetItem* it = subT->item(r, c);
            if (it && it->text().contains(QLatin1String("EEP"))) {
                foundEep = true;
                break;
            }
        }
    }
    QVERIFY2(foundEep, "subchannel table must contain an EEP row");

    // --- Tab 2 overview labels: real ensemble, FIG and subchannel counts ---
    QLabel* ensLabel = w.tab2EnsembleNameLabel();
    QVERIFY(ensLabel);
    const QString ensText = ensLabel->text();
    QVERIFY2(!ensText.contains(QLatin1String("Ready"))
                 && !ensText.contains(QLatin1String("Available")),
             qPrintable(QString("overview ensemble label must be real, got: %1").arg(ensText)));
    QVERIFY2(!ensText.contains(QLatin1String("(none)")),
             qPrintable(QString("overview ensemble must not be (none), got: %1").arg(ensText)));
    QVERIFY2(ensText.contains(QLatin1String("Bangkok DAB+"), Qt::CaseInsensitive),
             qPrintable(QString("overview ensemble label mismatch: %1").arg(ensText)));

    QLabel* ficLabel = w.tab2FicContentSummaryLabel();
    QVERIFY(ficLabel);
    const QString ficText = ficLabel->text();
    QVERIFY2(!ficText.contains(QLatin1String("Available"))
                 && !ficText.contains(QLatin1String("Ready")),
             qPrintable(QString("FIC summary must be real, got: %1").arg(ficText)));
    // "FIC Content: N FIGs" with N > 0
    const QRegularExpression figCountRe(QStringLiteral("FIC Content: (\\d+) FIGs"));
    const QRegularExpressionMatch figCountMatch = figCountRe.match(ficText);
    QVERIFY2(figCountMatch.hasMatch(),
             qPrintable(QString("FIC summary must carry a FIG count, got: %1").arg(ficText)));
    QVERIFY2(figCountMatch.captured(1).toInt() > 0,
             qPrintable(QString("FIC summary FIG count must be > 0, got: %1").arg(ficText)));

    QLabel* subLabel = w.tab2SubchannelOrgSummaryLabel();
    QVERIFY(subLabel);
    const QString subText = subLabel->text();
    QVERIFY2(!subText.contains(QLatin1String("Ready"))
                 && !subText.contains(QLatin1String("Available")),
             qPrintable(QString("subchannel summary must be real, got: %1").arg(subText)));
    QVERIFY2(subText.contains(QLatin1String("16")),
             qPrintable(QString("subchannel summary must show 16, got: %1").arg(subText)));

    // --- Tab 2 service table: real rows with visible labels ---
    QTableWidget* svcT = w.tab2ServiceTable();
    QVERIFY(svcT);
    QVERIFY2(svcT->rowCount() >= 1,
             qPrintable(QString("service table rows: %1").arg(svcT->rowCount())));
    bool foundSvcLabel = false;
    for (int r = 0; r < svcT->rowCount() && !foundSvcLabel; ++r) {
        QTableWidgetItem* labelItem = svcT->item(r, 1);
        if (labelItem && !labelItem->text().isEmpty()
            && labelItem->text() != QLatin1String("-")) {
            foundSvcLabel = true;
        }
    }
    QVERIFY2(foundSvcLabel, "service table must show real service labels");

    // --- Tab 3 FIG instance tracer: real chronological instances ---
    QTreeWidget* figTree = w.tab3FigInstanceTree();
    QVERIFY(figTree);
    QVERIFY2(figTree->topLevelItemCount() >= 1,
             qPrintable(QString("FIG instance rows: %1").arg(figTree->topLevelItemCount())));
    bool foundFig00 = false, foundFig10 = false;
    for (int i = 0; i < figTree->topLevelItemCount(); ++i) {
        const QString t = figTree->topLevelItem(i)->text(0);
        if (t.startsWith(QLatin1String("FIG 0/0"))) foundFig00 = true;
        if (t.startsWith(QLatin1String("FIG 1/0"))) foundFig10 = true;
    }
    QVERIFY2(foundFig00, "FIG 0/0 instances missing from the tracer");
    QVERIFY2(foundFig10, "FIG 1/0 instances missing from the tracer");

    // The details tree must show the real element tree of the pre-selected
    // instance (buildFIGInstanceTree selects row 0; the queued selection
    // sync rebuilds the details panel).
    QTreeWidget* figDetails = w.tab3FigItemDetailsTree();
    QVERIFY(figDetails);
    QTRY_VERIFY_WITH_TIMEOUT(figDetails->topLevelItemCount() > 0, 10000);
    const QString detailRoot = figDetails->topLevelItem(0)->text(0);
    QVERIFY2(detailRoot.startsWith(QLatin1String("FIG ")),
             qPrintable(QString("detail root must be a FIG, got: %1").arg(detailRoot)));

    // --- Finding 3: a real FIG 1/1 details row must decode the wire order
    // label@off(16 B) + flag@off+16. Scan the tracer's FIG 1/1 instances
    // until the RROne one (SId 0x2C61) is found: its details must show
    // "RROne FM 101" with character flag 0xF870.
    bool foundRROneLabel = false, foundF870 = false;
    for (int i = 0; i < figTree->topLevelItemCount() && !foundRROneLabel; ++i) {
        QTreeWidgetItem* row = figTree->topLevelItem(i);
        if (!row->text(0).startsWith(QLatin1String("FIG 1/1"))) {
            continue;
        }
        figTree->setCurrentItem(row);
        QCoreApplication::processEvents();
        QTRY_VERIFY_WITH_TIMEOUT(
            figDetails->topLevelItemCount() > 0 &&
                figDetails->topLevelItem(0)->text(0).startsWith(QLatin1String("FIG 1/1")),
            10000);
        std::function<void(const QTreeWidgetItem*)> walkDetails =
            [&](const QTreeWidgetItem* item) {
                for (int c = 0; c < item->childCount(); ++c) {
                    const QTreeWidgetItem* child = item->child(c);
                    const QString value = child->text(2).trimmed();
                    if (value == QLatin1String("RROne FM 101")) foundRROneLabel = true;
                    if (value.compare(QLatin1String("0xF870"), Qt::CaseInsensitive) == 0) {
                        foundF870 = true;
                    }
                    walkDetails(child);
                }
            };
        walkDetails(figDetails->topLevelItem(0));
    }
    if (!foundRROneLabel || !foundF870) {
        // Diagnostic: dump the details rows of the last selected FIG 1/1.
        std::function<void(const QTreeWidgetItem*, int)> dump =
            [&](const QTreeWidgetItem* item, int depth) {
                qInfo().noquote() << QString(depth * 2, ' ')
                                  << item->text(0) << "|" << item->text(2)
                                  << "|" << item->text(3);
                for (int c = 0; c < item->childCount(); ++c) {
                    dump(item->child(c), depth + 1);
                }
            };
        dump(figDetails->topLevelItem(0), 0);
    }
    QVERIFY2(foundRROneLabel,
             "FIG 1/1 details must show the service label 'RROne FM 101'");
    QVERIFY2(foundF870,
             "FIG 1/1 details must show the character flag 0xF870");

    // --- Finding 5: FIC overview fed with real analyzer values ---
    QTreeWidget* ficOverview = w.ficOverviewTree();
    QVERIFY(ficOverview);
    QVERIFY2(ficOverview->topLevelItemCount() > 0,
             "FIC overview tree must be populated");
    bool foundFicStream = false, foundFigCounts = false;
    bool noPlaceholder = true;
    std::function<void(const QTreeWidgetItem*)> walkOverview =
        [&](const QTreeWidgetItem* item) {
            const QString prop = item->text(0);
            const QString value = item->text(1);
            if (prop == QLatin1String("FIC stream")) foundFicStream = true;
            if (prop == QLatin1String("FIG counts by type")) foundFigCounts = true;
            if (value.contains(QLatin1String("Ready for data"))
                || value.contains(QLatin1String("Data Available"))) {
                noPlaceholder = false;
            }
            for (int c = 0; c < item->childCount(); ++c) {
                walkOverview(item->child(c));
            }
        };
    for (int i = 0; i < ficOverview->topLevelItemCount(); ++i) {
        walkOverview(ficOverview->topLevelItem(i));
    }
    QVERIFY2(foundFicStream, "FIC overview must show the real FIC stream row");
    QVERIFY2(foundFigCounts, "FIC overview must show FIG counts by type");
    QVERIFY2(noPlaceholder,
             "FIC overview must not contain placeholder literals");

    // --- Error & Stats: FIB CRC error counter fed from the analyzer ---
    QTableWidget* errT = w.errorCounterTable();
    QVERIFY(errT);
    int fibCrcRow = -1;
    for (int r = 0; r < errT->rowCount(); ++r) {
        QTableWidgetItem* typeItem = errT->item(r, 0);
        if (typeItem && typeItem->text() == QLatin1String("FIB CRC Errors")) {
            fibCrcRow = r;
            break;
        }
    }
    QVERIFY2(fibCrcRow >= 0, "FIB CRC Errors row missing from error counter");
    QTableWidgetItem* fibCrcCount = errT->item(fibCrcRow, 1);
    QVERIFY(fibCrcCount);
    QCOMPARE(fibCrcCount->text(), QStringLiteral("0"));  // clean capture

    w.close();
    QCoreApplication::processEvents();
}

QTEST_MAIN(TestGuiDockLayout)
#include "test_gui_dock_layout.moc"