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
 *      group structure matches the default layout) incl. layout_version=4
 *   f) stale v1/v2/v3 layout state (layout_version != 4) is discarded once so
 *      the first run after the upgrade shows the new default layout
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
#include <QBuffer>
#include <QImage>
#include <functional>
#include <algorithm>
#include <cstring>

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
    // T23: Input menu removed; ⚙ opens the UDP settings dialog
    void testUdpSettingsGearOpensDialog();
    void testPersistenceRoundTrip();
    void testLayoutVersionReset();
    void testFloatingRoundTrip();
    void testAutoHideConfig();
    void testReloadResetsAnalyzerState();
    void testEnsembleTreeRealFixture();
    // Manual-test findings 1-4 regression coverage
    void testTabTitlesAndStylesheet();
    void testTabPagesDescriptiveTitles();
    void testFrameNavigatorCrashGuard();
    void testNowPlayingDlsPlusSynthetic();
    void testDabPlusPadParserRejectsGarbage();
    // Review-window additions
    void testDlsResetHistory();
    void testLongUtf8LabelRoundTrip();
    void testDlPlusFromPadPath();
    void testXpadContinuationReuse();
    // T20 review findings 1 + 2
    void testXpadOversizePadRejected();
    void testDabPlusServiceCacheRevision();
    // T25: queued FIG signals must not fire against a torn-down analyzer
    void testTeardownDropsQueuedFigSignals();
    // T22 network-stream invalid-URL guard
    void testNetworkStreamInvalidUrlGuard();
    // T22 review F3a: post-connect stream errors are log-only
    void testStreamErrorAfterConnectedIsLogOnly();
    // MOT SlideShow (option A): X-PAD 12/13 adapter, panel, real fixture
    void testMotPadAdapterXpadPipeline();
    void testMotXpadCiFlagZeroContinuation();
    void testMotSlideshowPanelAndTabs();
    void testMotSlideshowRealFixture();

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
    // Observation (a): the Signal Analysis group must also have the tab ORDER
    // Ensemble Explorer -> Subchannel Organization -> CU Usage, and open on
    // Ensemble Explorer (not CU Usage), on a fresh layout.
    {
        ads::CDockWidget* ens = m1->findDockWidget(QStringLiteral("tab1_EnsembleExplorer"));
        QVERIFY(ens);
        ads::CDockAreaWidget* area = ens->dockAreaWidget();
        QVERIFY(area);
        const QList<ads::CDockWidget*> tabs = area->dockWidgets();
        QCOMPARE(tabs.size(), 3);
        QCOMPARE(tabs.at(0)->objectName(), QStringLiteral("tab1_EnsembleExplorer"));
        QCOMPARE(tabs.at(1)->objectName(), QStringLiteral("tab1_SubchannelOrg"));
        QCOMPARE(tabs.at(2)->objectName(), QStringLiteral("tab1_CUUsage"));
        QVERIFY(area->currentDockWidget());
        QCOMPARE(area->currentDockWidget()->objectName(),
                 QStringLiteral("tab1_EnsembleExplorer"));
        QCOMPARE(area->currentIndex(), 0);
    }
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

    // T23 (option A): the menu bar must have NO "Input" menu — the File/Network
    // radios below are the single source of truth for the input mode. The
    // unique Input-menu item (UDP Streaming Settings…) now lives behind the ⚙
    // button at the end of this network row.
    QMenuBar* menuBar = w.menuBar();
    QVERIFY(menuBar);
    bool hasInputMenu = false;
    for (QAction* a : menuBar->actions()) {
        if (a->text() == QLatin1String("Input")) hasInputMenu = true;
    }
    QVERIFY2(!hasInputMenu,
             "T23: the Input menu must be removed (menu bar is File / View / Help)");
    QToolButton* gear = w.udpSettingsButtonForTest();
    QVERIFY2(gear, "T23: ⚙ UDP-streaming-settings button must exist in the network row");
    QCOMPARE(gear->toolTip(), QStringLiteral("UDP streaming settings"));
    QCOMPARE(gear->text(), QStringLiteral("⚙"));

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

// ============================================================================
// T23 (option A): the ⚙ button in the network row opens the UDP Streaming
// Settings dialog that used to live in the removed Input menu. Exercises the
// non-modal construction path directly (no modal left open) and then the real
// click path with an offscreen auto-reject so the exec() loop cannot hang.
// ============================================================================
void TestGuiDockLayout::testUdpSettingsGearOpensDialog()
{
    clearSettingsStore();
    DABAnalyzerWindow w;
    w.show();
    QCoreApplication::processEvents();

    // 1) Non-modal construction: same dialog content as the old menu item.
    std::unique_ptr<QDialog> dlg(w.buildUdpSettingsDialogForTest());
    QVERIFY(dlg);
    QCOMPARE(dlg->windowTitle(), QStringLiteral("UDP Streaming Settings"));
    QVERIFY(dlg->findChild<QSpinBox*>(QStringLiteral("udpSettingsPort")));
    QVERIFY(dlg->findChild<QLineEdit*>(QStringLiteral("udpSettingsAddress")));
    QVERIFY(dlg->findChild<QCheckBox*>(QStringLiteral("udpSettingsMulticast")));
    dlg.reset();  // never leave a modal open

    // 2) The gear button is wired to the same handler: click it and auto-reject
    //    the modal as soon as it appears (offscreen, no user interaction).
    QToolButton* gear = w.udpSettingsButtonForTest();
    QVERIFY(gear);
    bool openedAndRejected = false;
    QTimer closer;
    closer.setInterval(20);
    QObject::connect(&closer, &QTimer::timeout, [&]() {
        for (QWidget* top : QApplication::topLevelWidgets()) {
            if (auto* d = qobject_cast<QDialog*>(top)) {
                if (d->windowTitle() == QStringLiteral("UDP Streaming Settings")
                    && d->isVisible()) {
                    openedAndRejected = true;
                    d->reject();
                    closer.stop();
                    return;
                }
            }
        }
    });
    closer.start();
    gear->click();
    QVERIFY2(openedAndRejected,
             "T23: clicking the ⚙ button must open the UDP Streaming Settings dialog");

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
    QCOMPARE(check.value(QStringLiteral("docking/layout_version")).toInt(), 4);
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

    // 1) First launch: hide Now Playing, select CU Usage in the Signal
    // Analysis group (the user-observed stale active tab), close -> saves state.
    {
        DABAnalyzerWindow w;
        w.show();
        QCoreApplication::processEvents();

        auto* m1 = w.findChild<ads::CDockManager*>("tab1DockManager");
        QVERIFY(m1);
        ads::CDockWidget* np = m1->findDockWidget(hiddenDockName);
        QVERIFY(np);
        np->toggleView(false);
        ads::CDockWidget* ens = m1->findDockWidget(QStringLiteral("tab1_EnsembleExplorer"));
        ads::CDockWidget* cu = m1->findDockWidget(QStringLiteral("tab1_CUUsage"));
        QVERIFY(ens && cu);
        ens->dockAreaWidget()->setCurrentDockWidget(cu);
        QCoreApplication::processEvents();
        QCOMPARE(ens->dockAreaWidget()->currentDockWidget()->objectName(),
                 QStringLiteral("tab1_CUUsage"));

        w.close(); // triggers closeEvent -> saveDockingState()
        QCoreApplication::processEvents();
    }
    {
        const QSettings check(QStringLiteral("StreamDAB-Analyzer"), QStringLiteral("DABAnalyzer"));
        QCOMPARE(check.value(versionKey).toInt(), 4);
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
        // Observation (a): the stale CU Usage active tab must be discarded; the
        // Signal Analysis group opens on DAB Ensemble Explorer.
        {
            ads::CDockWidget* ens = m1->findDockWidget(QStringLiteral("tab1_EnsembleExplorer"));
            QVERIFY(ens);
            QVERIFY(ens->dockAreaWidget());
            QVERIFY(ens->dockAreaWidget()->currentDockWidget());
            QCOMPARE(ens->dockAreaWidget()->currentDockWidget()->objectName(),
                     QStringLiteral("tab1_EnsembleExplorer"));
        }
        verifyTabbedGroup(m1, root,
                          {QStringLiteral("tab1_ETIOverview"),
                           QStringLiteral("tab1_Status"),
                           QStringLiteral("tab1_Timing")},
                          3, 2);

        auto* mBottom = w2.findChild<ads::CDockManager*>("bottomDockManager");
        QVERIFY(mBottom);
        QVERIFY(mBottom->findDockWidget(QStringLiteral("bottom_Messages")));

        w2.close(); // writes layout version 4 again
        QCoreApplication::processEvents();
    }

    // 4) Version is 4 again; a third launch restores normally (v4 round-trip).
    {
        const QSettings s(QStringLiteral("StreamDAB-Analyzer"), QStringLiteral("DABAnalyzer"));
        QCOMPARE(s.value(versionKey).toInt(), 4);
    }
    {
        DABAnalyzerWindow w3;
        w3.show();
        QCoreApplication::processEvents();

        auto* m1 = w3.findChild<ads::CDockManager*>("tab1DockManager");
        QVERIFY(m1);
        ads::CDockWidget* np = m1->findDockWidget(hiddenDockName);
        QVERIFY(np);
        QVERIFY(!np->isClosed());  // v4 state restored (Now Playing visible)

        w3.close();
        QCoreApplication::processEvents();
    }

    // 5) Real upgrade path: a stale v3 store (layout_version == 3, the MOT-
    // Slideshow predecessor) must also be discarded once, not only v1. Produce a
    // v3-tagged store whose saved layout differs (Now Playing hidden), then
    // confirm the next launch shows the default redesign layout and re-saves v4.
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
        w.close();  // writes layout_version 4 with Now Playing hidden
        QCoreApplication::processEvents();
    }
    {
        QSettings s(QStringLiteral("StreamDAB-Analyzer"), QStringLiteral("DABAnalyzer"));
        QCOMPARE(s.value(versionKey).toInt(), 4);
        QVERIFY(!s.value(QStringLiteral("docking/tab1")).isNull());
        s.setValue(versionKey, 3);  // simulate the v3 -> v4 migration
        s.sync();
    }
    {
        DABAnalyzerWindow w4;
        w4.show();
        QCoreApplication::processEvents();
        auto* m1 = w4.findChild<ads::CDockManager*>("tab1DockManager");
        QVERIFY(m1);
        ads::CDockWidget* np = m1->findDockWidget(hiddenDockName);
        QVERIFY(np);
        QVERIFY2(!np->isClosed() && np->isVisible(),
                 "stale v3 layout must be discarded: Now Playing visible in "
                 "the default redesign layout");
        w4.close();  // re-saves layout_version 4
        QCoreApplication::processEvents();
    }
    {
        const QSettings s(QStringLiteral("StreamDAB-Analyzer"), QStringLiteral("DABAnalyzer"));
        QCOMPARE(s.value(versionKey).toInt(), 4);
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
    // The Tab-3 instance tree reflects only the second load. Observation (d):
    // it is grouped by (type, ext), so count the CHILD instances, not the
    // top-level group nodes.
    int childInstances = 0;
    for (int i = 0; i < w.tab3FigInstanceTree()->topLevelItemCount(); ++i) {
        childInstances += w.tab3FigInstanceTree()->topLevelItem(i)->childCount();
    }
    QCOMPARE(childInstances, inst2);

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

    // --- Tab 3 FIG instance tracer: Observation (d) grouping ---
    // Top-level nodes are one (type, ext) GROUP each, labelled with the
    // instance count; instances are children carrying the collector index.
    QTreeWidget* figTree = w.tab3FigInstanceTree();
    QVERIFY(figTree);
    QVERIFY2(figTree->topLevelItemCount() >= 1,
             qPrintable(QString("FIG groups: %1").arg(figTree->topLevelItemCount())));
    bool foundFig00 = false, foundFig10 = false;
    bool allGroupsLabelled = true, anyChildren = false;
    for (int i = 0; i < figTree->topLevelItemCount(); ++i) {
        QTreeWidgetItem* group = figTree->topLevelItem(i);
        const QString t = group->text(0);
        if (t.startsWith(QLatin1String("FIG 0/0"))) foundFig00 = true;
        if (t.startsWith(QLatin1String("FIG 1/0"))) foundFig10 = true;
        // Group label carries a count in parentheses, e.g. "FIG 0/0  (1250)".
        if (!t.contains(QLatin1Char('(')) || !t.contains(QLatin1Char(')'))) {
            allGroupsLabelled = false;
        }
        // Every group node must be a group marker (UserRole < 0) and have
        // child instances with a non-negative collector index.
        QVERIFY2(group->data(0, Qt::UserRole).toInt() < 0,
                 qPrintable(QString("group node must be a group marker: %1").arg(t)));
        if (group->childCount() > 0) {
            anyChildren = true;
            QVERIFY(group->child(0)->data(0, Qt::UserRole).toInt() >= 0);
        }
    }
    QVERIFY2(foundFig00, "FIG 0/0 group missing from the tracer");
    QVERIFY2(foundFig10, "FIG 1/0 group missing from the tracer");
    QVERIFY2(allGroupsLabelled, "every FIG group must show its instance count in ()");
    QVERIFY2(anyChildren, "FIG groups must contain instance children");

    // The details tree must show the real element tree of the pre-selected
    // instance (buildFIGInstanceTree pre-selects the first CHILD; the queued
    // selection sync rebuilds the details panel).
    QTreeWidget* figDetails = w.tab3FigItemDetailsTree();
    QVERIFY(figDetails);
    QTRY_VERIFY_WITH_TIMEOUT(figDetails->topLevelItemCount() > 0, 10000);
    const QString detailRoot = figDetails->topLevelItem(0)->text(0);
    QVERIFY2(detailRoot.startsWith(QLatin1String("FIG ")),
             qPrintable(QString("detail root must be a FIG, got: %1").arg(detailRoot)));

    // Selecting a GROUP node must not crash; the details panel shows a group
    // summary (or otherwise leaves a safe state).
    {
        QTreeWidgetItem* group0 = figTree->topLevelItem(0);
        QVERIFY(group0);
        figTree->setCurrentItem(group0);
        QCoreApplication::processEvents();
        QTRY_VERIFY_WITH_TIMEOUT(figDetails->topLevelItemCount() > 0, 10000);
        QVERIFY2(figDetails->topLevelItem(0)->text(0).startsWith(QLatin1String("FIG ")),
                 "group selection must produce a safe FIG summary/details");
    }

    // --- Finding 3: a real FIG 1/1 details row must decode the wire order
    // label@off(16 B) + flag@off+16. Find the FIG 1/1 group and scan its
    // instance CHILDREN until the RROne one (SId 0x2C61) is found: its details
    // must show "RROne FM 101" with character flag 0xF870.
    bool foundRROneLabel = false, foundF870 = false;
    for (int g = 0; g < figTree->topLevelItemCount() && !foundRROneLabel; ++g) {
        QTreeWidgetItem* group = figTree->topLevelItem(g);
        if (!group->text(0).startsWith(QLatin1String("FIG 1/1"))) {
            continue;
        }
        for (int c = 0; c < group->childCount() && !foundRROneLabel; ++c) {
            QTreeWidgetItem* row = group->child(c);
            figTree->setCurrentItem(row);
            QCoreApplication::processEvents();
            QTRY_VERIFY_WITH_TIMEOUT(
                figDetails->topLevelItemCount() > 0 &&
                    figDetails->topLevelItem(0)->text(0).startsWith(QLatin1String("FIG 1/1")),
                10000);
            std::function<void(const QTreeWidgetItem*)> walkDetails =
                [&](const QTreeWidgetItem* item) {
                    for (int c2 = 0; c2 < item->childCount(); ++c2) {
                        const QTreeWidgetItem* child = item->child(c2);
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

    // =====================================================================
    // Manual-test finding 2: the frame navigator is only live AFTER the
    // synchronous parse loop finished, and selecting real frames decodes.
    // =====================================================================
    QVERIFY2(w.isFileLoaded(), "fixture must be marked loaded");
    QVERIFY2(!w.isProcessingForTest(), "parse loop must have finished");
    QCOMPARE(w.availableFrameCountForTest(), 5001);

    // (b) row 0 and the last row both decode hex/overview without a crash.
    w.selectFrameForTest(0);
    QCoreApplication::processEvents();
    QVERIFY2(!w.hexViewerTextForTest().isEmpty(),
             "hex viewer must show frame 0 after load");

    // Finding 5: m_frameDetails must be instantiated in the ETI Frame
    // Navigator and show the selected frame (it was a dead null widget before).
    QVERIFY2(!w.frameDetailsTextForTest().isEmpty(),
             "frame details label must be instantiated and populated");
    QVERIFY2(w.frameDetailsTextForTest().contains(QStringLiteral("Selected Frame: 1")),
             qPrintable(QString("frame details must show frame 1, got: %1")
                            .arg(w.frameDetailsTextForTest())));

    auto* frameList = w.frameListForTest();
    QVERIFY(frameList);
    QVERIFY2(frameList->count() > 0, "frame list must be populated after load");
    const int lastRow = frameList->count() - 1;
    w.selectFrameForTest(lastRow);
    QCoreApplication::processEvents();
    QVERIFY2(!w.hexViewerTextForTest().isEmpty(),
             "hex viewer must show the last frame after load");

    // (c) a row beyond the available frames is ignored safely (no crash, no
    // widget mutation).
    const QString beforeOutOfRange = w.hexViewerTextForTest();
    w.selectFrameForTest(w.availableFrameCountForTest() + 100);
    QCoreApplication::processEvents();
    QCOMPARE(w.hexViewerTextForTest(), beforeOutOfRange);

    // =====================================================================
    // Manual-test finding 3: Now Playing (DLS+) shows a documented valid
    // state after a real capture load (real metadata OR the empty state).
    // =====================================================================
    const int dlsCount = w.dlsPlusMessageCountForTest();
    qInfo() << "[GUI-TEST] DLS+ messages decoded over the full Bangkok capture:"
            << dlsCount;
    QVERIFY2(dlsCount > 0, "the DLS+ pipeline must decode labels from the fixture");

    // Known real label check + UTF-8/Thai integrity (no replacement chars).
    const QStringList labelsSeen = w.dlsLabelsSeenForTest();
    QVERIFY2(!labelsSeen.isEmpty(), "decoded DLS labels must be recorded");
    bool foundKnownAsciiLabel = false, foundThaiLabel = false, anyReplacementChar = false;
    const QString thaiMarker = QStringLiteral("\u0E2A\u0E16\u0E32\u0E19\u0E35\u0E27\u0E34\u0E17\u0E22\u0E38");  // "สถานีวิทยุ"
    for (const QString& label : labelsSeen) {
        if (label.contains(QStringLiteral("EFM94"), Qt::CaseInsensitive)) {
            foundKnownAsciiLabel = true;
        }
        if (label.contains(thaiMarker)) {
            foundThaiLabel = true;
        }
        if (label.contains(QChar(0xFFFD))) {
            anyReplacementChar = true;
        }
    }
    QVERIFY2(foundKnownAsciiLabel,
             qPrintable(QString("known fixture DLS label 'EFM94' not found; %1 labels: %2")
                            .arg(labelsSeen.size())
                            .arg(labelsSeen.mid(0, qMin(8, labelsSeen.size())).join(QStringLiteral(" | ")))));
    QVERIFY2(foundThaiLabel,
             "the Thai DLS label (สถานีวิทยุ…) must decode exactly (UTF-8)");
    QVERIFY2(!anyReplacementChar,
             "no decoded DLS label may contain U+FFFD (UTF-8 / Thai integrity)");

    const QString nowPlayingStatus = w.nowPlayingStatusForTest();
    if (dlsCount > 0) {
        QVERIFY2(nowPlayingStatus.startsWith(QStringLiteral("Updated:")),
                 qPrintable(QString("Now Playing must show real metadata, got: %1")
                                .arg(nowPlayingStatus)));
    } else {
        QVERIFY2(nowPlayingStatus == QStringLiteral("No metadata received"),
                 qPrintable(QString("Now Playing must show the documented empty state, got: %1")
                                .arg(nowPlayingStatus)));
    }

    w.close();
    QCoreApplication::processEvents();
}

// ============================================================================
// Manual-test finding 4: dock titles + finding 1: tab-bar stylesheet
// ============================================================================
void TestGuiDockLayout::testTabTitlesAndStylesheet()
{
    clearSettingsStore();
    DABAnalyzerWindow w;
    w.show();
    QCoreApplication::processEvents();

    // Finding 4: descriptive dock titles. objectNames are unchanged (asserted
    // by test25DocksWithExpectedNames), only the displayed titles changed.
    auto* m2 = w.findChild<ads::CDockManager*>("tab2DockManager");
    QVERIFY(m2);
    ads::CDockWidget* d = nullptr;
    d = m2->findDockWidget(QStringLiteral("tab2_Left"));
    QVERIFY(d);
    QCOMPARE(d->windowTitle(), QStringLiteral("Service Tree"));
    d = m2->findDockWidget(QStringLiteral("tab2_Center"));
    QVERIFY(d);
    QCOMPARE(d->windowTitle(), QStringLiteral("Ensemble & Sub-Channel Tables"));
    d = m2->findDockWidget(QStringLiteral("tab2_Right"));
    QVERIFY(d);
    QCOMPARE(d->windowTitle(), QStringLiteral("FIG Content"));

    auto* m3 = w.findChild<ads::CDockManager*>("tab3DockManager");
    QVERIFY(m3);
    d = m3->findDockWidget(QStringLiteral("tab3_Left"));
    QVERIFY(d);
    QCOMPARE(d->windowTitle(), QStringLiteral("FIG Instance List"));
    d = m3->findDockWidget(QStringLiteral("tab3_Right"));
    QVERIFY(d);
    QCOMPARE(d->windowTitle(), QStringLiteral("FIG Item Details"));

    // Finding 1: the main tab bar carries the accent-highlight stylesheet.
    auto* mainTabs = w.findChild<QTabWidget*>(QStringLiteral("mainTabs"));
    QVERIFY(mainTabs);
    const QString qss = mainTabs->styleSheet();
    QVERIFY2(qss.contains(QStringLiteral("#4a8fd4")),
             qPrintable(QString("main tab stylesheet missing accent: %1").arg(qss)));
    QVERIFY2(qss.contains(QStringLiteral("QTabBar::tab:selected")),
             qPrintable(QString("main tab stylesheet missing selected tab rule: %1").arg(qss)));
    QVERIFY2(qss.contains(QStringLiteral("palette(windowText)")),
             qPrintable(QString("main tab stylesheet must derive text from palette: %1").arg(qss)));

    w.close();
    QCoreApplication::processEvents();
}

// ============================================================================
// Manual-test finding 2: frame-navigator click crash guard
// ============================================================================
void TestGuiDockLayout::testFrameNavigatorCrashGuard()
{
    clearSettingsStore();
    DABAnalyzerWindow w;
    w.show();
    QCoreApplication::processEvents();

    auto* frameList = w.frameListForTest();
    QVERIFY(frameList);
    QVERIFY(frameList->isEnabled());
    QVERIFY2(w.hexViewerTextForTest().isEmpty(),
             "hex viewer must start empty before any frame is selected");

    // (a) Isolated crash guard: m_fileLoaded=true WITH m_processing=true. The
    // file-loaded state must not let a click slip through while parsing, and
    // the frame list must be disabled during processing.
    w.setFileLoadedForTest(true);
    w.beginProcessingForTest();
    QVERIFY(w.isFileLoaded());
    QVERIFY(w.isProcessingForTest());
    QVERIFY2(!frameList->isEnabled(),
             "frame list must be disabled while parsing");
    w.selectFrameForTest(0);
    w.selectFrameForTest(999999);
    QCoreApplication::processEvents();
    QVERIFY2(w.hexViewerTextForTest().isEmpty(),
             "no frame data may be rendered while parsing");

    // (b) The worker-thread error path must ALWAYS re-arm the guard: processing
    // cleared and the frame list re-enabled (Finding 8).
    w.simulateWorkerErrorForTest(QStringLiteral("simulated worker failure"));
    QCoreApplication::processEvents();
    QVERIFY2(!w.isProcessingForTest(),
             "processing flag must be cleared after a worker error");
    QVERIFY2(frameList->isEnabled(),
             "frame list must be re-enabled after a worker error");

    // (c) Guard re-armed but no frames loaded -> an out-of-range selection is
    // ignored safely, no render, no crash.
    w.selectFrameForTest(0);
    QCoreApplication::processEvents();
    QVERIFY2(w.hexViewerTextForTest().isEmpty(),
             "no frame data may be rendered when no frames are available");

    // (d) Even with the processing flag cleared, an unloaded window still
    // refuses to decode (m_fileLoaded is false) -> "no file loaded" branch.
    w.setFileLoadedForTest(false);
    w.selectFrameForTest(0);
    QCoreApplication::processEvents();
    QVERIFY2(w.hexViewerTextForTest().isEmpty(),
             "no frame data may be rendered without a loaded file");

    w.close();
    QCoreApplication::processEvents();
}

// ============================================================================
// Refinement A: standard DAB+ superframe/AU/PAD -> DLS -> Now Playing wiring
// ============================================================================
namespace {

// Build a minimal but structurally valid DAB+ superframe (ETSI TS 102 563)
// carrying one DSE/PAD with a variable-size X-PAD whose Data Group is a
// single First+Last DLS segment "Hey Jude" (charset 15). This drives the REAL
// testGui DAB+ parser (dabFirecodeCrc + AU CRC + F-PAD/X-PAD + DLS assembly),
// not a hand-built decoder input.
QByteArray buildSyntheticDabPlusDlsSuperframe()
{
    constexpr int sfLen = 120;           // subchIndex = 1 (sfLen % 120 == 0)
    constexpr int auStart[5] = {8, 30, 50, 70, 110};
    QByteArray sf(sfLen, '\0');
    auto* b = reinterpret_cast<uint8_t*>(sf.data());

    // --- DLS Data Group: prefix(1: First|Last|len-1) + prefix(1: charset) +
    //     text + CRC16 (~CCITT-FALSE over prefix+text) ---
    const QByteArray text = "Hey Jude";
    const int fieldLen = text.size();  // <= 16 (single segment)
    QByteArray dg;
    dg.append(static_cast<char>(0x40 | 0x20 | ((fieldLen - 1) & 0x0F)));
    dg.append(static_cast<char>(0xF0));  // charset 15 (UTF-8), segment 0
    dg.append(text);
    const uint16_t dgCrc = static_cast<uint16_t>(~eti::crc16ccitt_false(
        reinterpret_cast<const uint8_t*>(dg.constData()), dg.size()));
    dg.append(static_cast<char>((dgCrc >> 8) & 0xFF));
    dg.append(static_cast<char>(dgCrc & 0xFF));

    // --- Variable-size X-PAD (logical): CI(type 2 = DL start, 12 bytes) +
    //     end marker + data subfield; transmitted reversed ---
    QByteArray logicalXpad;
    logicalXpad.append(static_cast<char>((3 << 5) | 2));  // len_index 3 -> 12
    logicalXpad.append(static_cast<char>(0x00));          // CI end marker
    logicalXpad.append(dg);
    QByteArray onWireXpad = logicalXpad;
    std::reverse(onWireXpad.begin(), onWireXpad.end());

    // --- F-PAD: type 0, variable X-PAD, CI flag set ---
    QByteArray padRegion = onWireXpad;
    padRegion.append(static_cast<char>(0x20));
    padRegion.append(static_cast<char>(0x02));

    // --- First AU payload: DSE (id 4) + pad length + PAD region ---
    QByteArray au0;
    au0.append(static_cast<char>(0x80));                     // DSE element id 4
    au0.append(static_cast<char>(padRegion.size()));         // pad_len
    au0.append(padRegion);

    // --- Superframe header: format byte 0 -> 4 AUs, first AU at 8 ---
    b[2] = 0x00;
    b[3] = 0x01;  // au 1 = 0x01E = 30
    b[4] = 0xE0;  // au 2 = 0x032 = 50
    b[5] = 0x32;
    b[6] = 0x04;  // au 3 = 0x046 = 70
    b[7] = 0x60;

    // --- Write AUs (payload + trailing CRC16), then the FireCode (which
    //     covers b[2..10], overlapping the first 3 bytes of AU0) ---
    for (int au = 0; au < 4; ++au) {
        const int payloadLen = auStart[au + 1] - auStart[au] - 2;
        QByteArray payload(payloadLen, '\0');
        if (au == 0) {
            payload = au0;
            payload.resize(payloadLen, '\0');
        }
        const uint16_t auCrc = static_cast<uint16_t>(~eti::crc16ccitt_false(
            reinterpret_cast<const uint8_t*>(payload.constData()), payload.size()));
        std::memcpy(b + auStart[au], payload.constData(),
                    static_cast<size_t>(payloadLen));
        b[auStart[au + 1] - 2] = static_cast<uint8_t>(auCrc >> 8);
        b[auStart[au + 1] - 1] = static_cast<uint8_t>(auCrc & 0xFF);
    }

    const uint16_t fc = dabFirecodeCrc(b + 2, 9);
    b[0] = static_cast<uint8_t>(fc >> 8);
    b[1] = static_cast<uint8_t>(fc & 0xFF);
    return sf;
}

}  // namespace

void TestGuiDockLayout::testNowPlayingDlsPlusSynthetic()
{
    clearSettingsStore();
    DABAnalyzerWindow w;
    w.show();
    QCoreApplication::processEvents();

    // Documented initial state before any data.
    QCOMPARE(w.nowPlayingTrackForTest(), QStringLiteral("--"));
    QCOMPARE(w.nowPlayingStatusForTest(), QStringLiteral("Waiting for DLS+…"));

    // Drive the REAL DAB+ superframe/AU/PAD parser with a synthetic AU whose
    // DSE carries an X-PAD DLS data group.
    const QByteArray sf = buildSyntheticDabPlusDlsSuperframe();
    QVERIFY2(w.runDabPlusDlsParserForTest(sf),
             "the standard DAB+ parser must assemble a DLS label from the "
             "synthetic superframe");
    QCOMPARE(w.dlsPlusLabelsAssembledForTest(), 1);
    QCOMPARE(w.dlsPlusMessageCountForTest(), 1);

    // nowPlayingUpdated() -> updateNowPlaying() must update the panel.
    QCOMPARE(w.nowPlayingTrackForTest(), QStringLiteral("Hey Jude"));
    QVERIFY2(w.nowPlayingStatusForTest().startsWith(QStringLiteral("Updated:")),
             qPrintable(QString("Now Playing status must show the update: %1")
                            .arg(w.nowPlayingStatusForTest())));

    // Second: the DL+ tag path (ITEM.ARTIST / ITEM.TITLE / ITEM.ALBUM), using
    // the same vector semantics as tests/test_dls_plus_decoder.cpp. The
    // fixture carries plain DLS (0 DL+ segments), so this covers the DL+ tag
    // mapping -> decoder -> panel chain.
    QVector<QPair<int, QPair<int, int>>> tags;
    tags.append({4, {0, 10}});   // ITEM.ARTIST "The Beatles"
    tags.append({1, {14, 7}});   // ITEM.TITLE  "Hey Jude"
    tags.append({2, {24, 9}});   // ITEM.ALBUM  "Abbey Road"
    int mapped = 0;
    const QByteArray dlPlusPad = DABAnalyzerWindow::buildDecoderPadForTest(
        QByteArrayLiteral("The Beatles - Hey Jude [Abbey Road]"), 15, tags, mapped);
    QCOMPARE(mapped, 3);
    QVERIFY(w.feedPadDataForTest(dlPlusPad));
    QCOMPARE(w.nowPlayingArtistForTest(), QStringLiteral("The Beatles"));
    QCOMPARE(w.nowPlayingTrackForTest(), QStringLiteral("Hey Jude"));
    QCOMPARE(w.nowPlayingAlbumForTest(), QStringLiteral("Abbey Road"));

    w.close();
    QCoreApplication::processEvents();
}

void TestGuiDockLayout::testDabPlusPadParserRejectsGarbage()
{
    clearSettingsStore();
    DABAnalyzerWindow w;
    w.show();
    QCoreApplication::processEvents();

    // A superframe with a valid FireCode but no PAD DSE must not produce a
    // label (and must not crash).
    QByteArray sf(120, '\0');
    auto* b = reinterpret_cast<uint8_t*>(sf.data());
    b[2] = 0x00;
    b[3] = 0x01;
    b[4] = 0xE0;
    b[5] = 0x32;
    b[6] = 0x04;
    b[7] = 0x60;
    const uint16_t fc = dabFirecodeCrc(b + 2, 9);
    b[0] = static_cast<uint8_t>(fc >> 8);
    b[1] = static_cast<uint8_t>(fc & 0xFF);

    QVERIFY(!w.runDabPlusDlsParserForTest(sf));
    QCOMPARE(w.dlsPlusLabelsAssembledForTest(), 0);
    QCOMPARE(w.nowPlayingTrackForTest(), QStringLiteral("--"));

    w.close();
    QCoreApplication::processEvents();
}

// ============================================================================
// Finding 6: DLSPlusDecoder::resetHistory() clears the change-detection
// baseline so a second load cannot suppress its first (identical) label.
// ============================================================================
void TestGuiDockLayout::testDlsResetHistory()
{
    eti::dls_plus::DLSPlusDecoder decoder;
    QSignalSpy spy(&decoder, &eti::dls_plus::DLSPlusDecoder::nowPlayingUpdated);
    QVERIFY2(spy.isValid(), "nowPlayingUpdated spy must be valid");

    int mapped = 0;
    const QByteArray pad = DABAnalyzerWindow::buildDecoderPadForTest(
        QByteArrayLiteral("Same Artist - Same Song"), 15, {}, mapped);
    QCOMPARE(mapped, 0);  // plain DLS -> fallback descriptor

    QVERIFY(decoder.processPADData(pad));
    QCOMPARE(spy.count(), 1);
    // Identical message -> change detection suppresses the signal.
    QVERIFY(decoder.processPADData(pad));
    QCOMPARE(spy.count(), 1);
    // resetHistory() clears the baseline -> the next identical label emits.
    decoder.resetHistory();
    QVERIFY(decoder.processPADData(pad));
    QCOMPARE(spy.count(), 2);
}

// ============================================================================
// Finding 1: a >=64-byte UTF-8/Thai label must round-trip exactly through the
// plain-DLS adapter (the fallback descriptor must keep length bit 6).
// ============================================================================
void TestGuiDockLayout::testLongUtf8LabelRoundTrip()
{
    clearSettingsStore();
    DABAnalyzerWindow w;
    w.show();
    QCoreApplication::processEvents();

    // 20 Thai 'ก' (3 bytes each = 60) + "Hello!" (6) = 66 bytes: >64 so the
    // 7-bit length field's bit 6 must be preserved, and <=127 so none of the
    // label is intentionally truncated.
    const QString thai = QString(20, QChar(0x0E01)) + QStringLiteral("Hello!");
    const QByteArray labelBytes = thai.toUtf8();
    QVERIFY2(labelBytes.size() > 64 && labelBytes.size() <= 127,
             qPrintable(QString("test label must be 65..127 bytes, got %1")
                            .arg(labelBytes.size())));

    int mapped = 0;
    const QByteArray pad = DABAnalyzerWindow::buildDecoderPadForTest(labelBytes, 15, {}, mapped);
    QCOMPARE(mapped, 0);
    QVERIFY(!pad.isEmpty());

    QVERIFY(w.feedPadDataForTest(pad));
    const QString track = w.nowPlayingTrackForTest();
    QCOMPARE(track, thai);
    QVERIFY2(!track.contains(QChar(0xFFFD)),
             "long UTF-8/Thai label must not contain U+FFFD");
    QCOMPARE(track.toUtf8().size(), labelBytes.size());  // no truncation

    // Boundary table for the 7-bit length field (bit 6 of `length - 1`/length
    // lives in descriptor byte 2 bit 0): 63 = 0b0111111, 64 = bit 6 set with
    // low 6 bits 0 (the case the fix targeted), 65, and 127. ASCII keeps byte
    // count == character count so the round-trip is exact.
    for (const int n : {63, 64, 65, 127}) {
        const QByteArray bytes(n, 'A');
        int m = 0;
        const QByteArray p = DABAnalyzerWindow::buildDecoderPadForTest(bytes, 15, {}, m);
        QCOMPARE(m, 0);
        QVERIFY(!p.isEmpty());
        QVERIFY(w.feedPadDataForTest(p));
        const QString got = w.nowPlayingTrackForTest();
        QCOMPARE(got.toUtf8().size(), n);
        QCOMPARE(got, QString::fromUtf8(bytes));
    }
    // >127 is intentionally truncated to the 7-bit field maximum (127).
    {
        const QByteArray bytes(130, 'B');
        int m = 0;
        const QByteArray p = DABAnalyzerWindow::buildDecoderPadForTest(bytes, 15, {}, m);
        QVERIFY(!p.isEmpty());
        QVERIFY(w.feedPadDataForTest(p));
        const QString got = w.nowPlayingTrackForTest();
        QCOMPARE(got.toUtf8().size(), 127);
        QCOMPARE(got, QString::fromUtf8(bytes.left(127)));
    }

    w.close();
    QCoreApplication::processEvents();
}

// ============================================================================
// Review: exercise the real PAD path (F-PAD/X-PAD -> Data Group command 0x02)
// rather than only the decoder-boundary builder.
// ============================================================================
void TestGuiDockLayout::testDlPlusFromPadPath()
{
    clearSettingsStore();
    DABAnalyzerWindow w;
    w.show();
    QCoreApplication::processEvents();

    // DLS text "Hi" as a First+Last DLS Data Group (prefix + charset + text + CRC).
    const QByteArray dlsText = QByteArrayLiteral("Hi");
    QByteArray dlsGroup;
    dlsGroup.append(static_cast<char>(0x40 | 0x20 | (dlsText.size() - 1)));  // First|Last|len-1
    dlsGroup.append(static_cast<char>(0xF0));  // charset 15, segment 0
    dlsGroup.append(dlsText);
    uint16_t crc = static_cast<uint16_t>(~eti::crc16ccitt_false(
        reinterpret_cast<const uint8_t*>(dlsGroup.constData()), dlsGroup.size()));
    dlsGroup.append(static_cast<char>((crc >> 8) & 0xFF));
    dlsGroup.append(static_cast<char>(crc & 0xFF));  // 6 bytes total

    // DL+ command Data Group (command id 0x02) with one ITEM.TITLE tag [1,0,1]
    // tagging DLS bytes 0..1 ("Hi").
    QByteArray dlPlusGroup;
    dlPlusGroup.append(static_cast<char>(0x40 | 0x20 | 0x10 | 0x02));  // seg 0|last|command|id 2
    dlPlusGroup.append(static_cast<char>(0xF3));  // charset 15, field length = 4
    dlPlusGroup.append(static_cast<char>(0x00));  // number of tags - 1 = 0
    dlPlusGroup.append(static_cast<char>(1));     // ITEM.TITLE content type
    dlPlusGroup.append(static_cast<char>(0));     // start marker
    dlPlusGroup.append(static_cast<char>(1));     // length marker - 1 = 1
    crc = static_cast<uint16_t>(~eti::crc16ccitt_false(
        reinterpret_cast<const uint8_t*>(dlPlusGroup.constData()), dlPlusGroup.size()));
    dlPlusGroup.append(static_cast<char>((crc >> 8) & 0xFF));
    dlPlusGroup.append(static_cast<char>(crc & 0xFF));  // 8 bytes total

    // Variable-size X-PAD CI list: DL+ group (len index 2 -> 8) then DLS group
    // (len index 1 -> 6), terminated by the end marker.
    QByteArray logicalXpad;
    logicalXpad.append(static_cast<char>((2 << 5) | 2));  // CI: type 2, 8 bytes
    logicalXpad.append(static_cast<char>((1 << 5) | 2));  // CI: type 2, 6 bytes
    logicalXpad.append(static_cast<char>(0x00));          // end marker
    logicalXpad.append(dlPlusGroup);
    logicalXpad.append(dlsGroup);

    QByteArray onWireXpad = logicalXpad;
    std::reverse(onWireXpad.begin(), onWireXpad.end());

    QVERIFY2(w.runRawXpadDlsParserForTest(onWireXpad),
             "PAD DL+ command 0x02 must assemble a label with its tag");
    QCOMPARE(w.dlsPlusLabelsAssembledForTest(), 1);
    QCOMPARE(w.nowPlayingTrackForTest(), QStringLiteral("Hi"));
    QVERIFY2(w.nowPlayingStatusForTest().startsWith(QStringLiteral("Updated:")),
             qPrintable(QString("Now Playing status must show the update: %1")
                            .arg(w.nowPlayingStatusForTest())));

    w.close();
    QCoreApplication::processEvents();
}

// ============================================================================
// Finding 2/3: a ci_flag==0 AU must reuse the previous CI list and APPEND to
// the data-group assembly (never reset it) so a group split across AUs
// reassembles.
// ============================================================================
void TestGuiDockLayout::testXpadContinuationReuse()
{
    const QByteArray text = QByteArrayLiteral("ABCDEFGH");
    // DLS Data Group: prefix(First|Last|len-1) + charset + text + CRC = 12 bytes.
    QByteArray group;
    group.append(static_cast<char>(0x40 | 0x20 | (text.size() - 1)));
    group.append(static_cast<char>(0xF0));  // charset 15, segment 0
    group.append(text);
    const uint16_t crc = static_cast<uint16_t>(~eti::crc16ccitt_false(
        reinterpret_cast<const uint8_t*>(group.constData()), group.size()));
    group.append(static_cast<char>((crc >> 8) & 0xFF));
    group.append(static_cast<char>(crc & 0xFF));
    QCOMPARE(group.size(), 12);

    const QByteArray firstHalf = group.left(6);
    const QByteArray secondHalf = group.mid(6);

    DabPlusSubchannelParser parser;

    // AU 1: ci_flag=1, variable X-PAD announcing a 6-byte subfield (CI type 2).
    QByteArray logical1;
    logical1.append(static_cast<char>((1 << 5) | 2));  // CI: type 2, len index 1 -> 6
    logical1.append(static_cast<char>(0x00));          // end marker
    logical1.append(firstHalf);
    QByteArray wire1 = logical1;
    std::reverse(wire1.begin(), wire1.end());
    const uint8_t fpadCi[2] = {0x20, 0x02};  // type 0, variable, CI set
    DlsAssembly assembly1;
    QVERIFY2(!parser.processPad(reinterpret_cast<const uint8_t*>(wire1.constData()),
                                static_cast<size_t>(wire1.size()), fpadCi, assembly1),
             "the first half of a split data group must not complete yet");

    // AU 2: ci_flag=0, NO CI bytes, carrying the remaining 6 bytes. The parser
    // must reuse the previous CI list and append (not reset) the assembly.
    QByteArray wire2 = secondHalf;
    std::reverse(wire2.begin(), wire2.end());
    const uint8_t fpadNoCi[2] = {0x20, 0x00};  // type 0, variable, CI clear
    DlsAssembly assembly2;
    QVERIFY2(parser.processPad(reinterpret_cast<const uint8_t*>(wire2.constData()),
                               static_cast<size_t>(wire2.size()), fpadNoCi, assembly2),
             "ci_flag==0 continuation must reuse the previous CI list");
    QVERIFY2(assembly2.hasLabel, "the reassembled continuation must yield a DLS label");
    QCOMPARE(assembly2.dlText, text);

    // --- ci_flag==1, type 3 APPEND: a group carried by a type-2 start followed
    // by type-3 continuations must reassemble (type 3 never resets the buffer).
    {
        DabPlusSubchannelParser p3;
        const uint8_t fpadCi[2] = {0x20, 0x02};  // type 0, variable X-PAD, CI set
        const auto type3Pad = [](const QByteArray& chunk) {
            QByteArray logical;
            logical.append(static_cast<char>((0 << 5) | 3));  // CI: type 3, len index 0 -> 4
            logical.append(static_cast<char>(0x00));          // end marker
            logical.append(chunk);
            std::reverse(logical.begin(), logical.end());
            return logical;
        };

        // AU 1: type 2 starts the group (first 4 of 12 bytes).
        QByteArray logical1;
        logical1.append(static_cast<char>((0 << 5) | 2));  // CI: type 2, 4 bytes
        logical1.append(static_cast<char>(0x00));
        logical1.append(group.left(4));
        std::reverse(logical1.begin(), logical1.end());
        DlsAssembly a1;
        QVERIFY2(!p3.processPad(reinterpret_cast<const uint8_t*>(logical1.constData()),
                                static_cast<size_t>(logical1.size()), fpadCi, a1),
                 "a partial data group must not complete");

        // AU 2: type 3 continues (middle 4 bytes).
        const QByteArray wire2 = type3Pad(group.mid(4, 4));
        DlsAssembly a2;
        QVERIFY2(!p3.processPad(reinterpret_cast<const uint8_t*>(wire2.constData()),
                                static_cast<size_t>(wire2.size()), fpadCi, a2),
                 "middle of a split data group must not complete yet");

        // AU 3: type 3 continues (last 4 bytes) -> completes.
        const QByteArray wire3 = type3Pad(group.mid(8, 4));
        DlsAssembly a3;
        QVERIFY2(p3.processPad(reinterpret_cast<const uint8_t*>(wire3.constData()),
                               static_cast<size_t>(wire3.size()), fpadCi, a3),
                 "type-3 continuation must append and complete the group");
        QVERIFY(a3.hasLabel);
        QCOMPARE(a3.dlText, text);
    }

    // --- A new type-2 (data-group start) must DISCARD an in-progress buffer:
    // feed a partial group, then a complete different group; the result must be
    // the complete group, proving the partial bytes were dropped, not merged.
    {
        DabPlusSubchannelParser p4;
        const uint8_t fpadCi[2] = {0x20, 0x02};
        const auto type2Pad = [](const QByteArray& chunk, int lenIndex) {
            QByteArray logical;
            logical.append(static_cast<char>((lenIndex << 5) | 2));
            logical.append(static_cast<char>(0x00));  // end marker
            logical.append(chunk);
            std::reverse(logical.begin(), logical.end());
            return logical;
        };
        const auto makeGroup = [](const QByteArray& eight) {
            QByteArray g;
            g.append(static_cast<char>(0x40 | 0x20 | (eight.size() - 1)));  // First|Last|len-1
            g.append(static_cast<char>(0xF0));                              // charset 15, seg 0
            g.append(eight);
            const uint16_t c = static_cast<uint16_t>(~eti::crc16ccitt_false(
                reinterpret_cast<const uint8_t*>(g.constData()), g.size()));
            g.append(static_cast<char>((c >> 8) & 0xFF));
            g.append(static_cast<char>(c & 0xFF));
            return g;
        };
        const QByteArray groupA = makeGroup(QByteArrayLiteral("ABCDEFGH"));
        QCOMPARE(groupA.size(), 12);
        const QByteArray wirePartial = type2Pad(groupA.left(8), 2);  // len index 2 -> 8
        DlsAssembly ignored;
        QVERIFY2(!p4.processPad(reinterpret_cast<const uint8_t*>(wirePartial.constData()),
                                static_cast<size_t>(wirePartial.size()), fpadCi, ignored),
                 "partial group must not complete");

        const QByteArray groupB = makeGroup(QByteArrayLiteral("BBBBBBBB"));
        const QByteArray wireFull = type2Pad(groupB, 3);  // len index 3 -> 12
        DlsAssembly done;
        QVERIFY2(p4.processPad(reinterpret_cast<const uint8_t*>(wireFull.constData()),
                               static_cast<size_t>(wireFull.size()), fpadCi, done),
                 "a new type-2 must start a fresh group and complete it");
        QVERIFY(done.hasLabel);
        QCOMPARE(done.dlText, QByteArrayLiteral("BBBBBBBB"));
    }
}

// ============================================================================
// T20 finding 1: an AU whose DSE announces a long PAD but carries a short CI
// list (announced ~14, xpadLen ~253) used to index xpad[] past its 196-byte
// buffer in the trailing-padding scan. The parser must reject it (no OOB, no
// crash) and must stay usable afterwards.
// ============================================================================
void TestGuiDockLayout::testXpadOversizePadRejected()
{
    DabPlusSubchannelParser parser;

    // Build a complete, otherwise-decodable DLS group (12 bytes, "ABCDEFGH").
    const QByteArray text = QByteArrayLiteral("ABCDEFGH");
    QByteArray group;
    group.append(static_cast<char>(0x40 | 0x20 | (text.size() - 1)));  // First|Last|len-1
    group.append(static_cast<char>(0xF0));                            // charset 15, seg 0
    group.append(text);
    const uint16_t dgCrc = static_cast<uint16_t>(~eti::crc16ccitt_false(
        reinterpret_cast<const uint8_t*>(group.constData()), group.size()));
    group.append(static_cast<char>((dgCrc >> 8) & 0xFF));
    group.append(static_cast<char>(dgCrc & 0xFF));
    QCOMPARE(group.size(), 12);

    // Oversized X-PAD (253 > the fixed 196-byte buffer) whose announced CI+data
    // (end marker + 12-byte sub-field = 14) is far below the actual length; the
    // padding tail is zero, so WITHOUT the size guard this would scan past the
    // buffer and then decode the group (return true). WITH the guard it must be
    // rejected up front.
    const size_t xpadLen = 253;
    QByteArray logical(static_cast<int>(xpadLen), '\0');
    logical[0] = static_cast<char>((3 << 5) | 2);  // CI: type 2, len index 3 -> 12
    logical[1] = static_cast<char>(0x00);          // CI end marker
    std::memcpy(logical.data() + 2, group.constData(), static_cast<size_t>(group.size()));
    // On-wire X-PAD is the reverse of the logical order.
    QByteArray onWire = logical;
    std::reverse(onWire.begin(), onWire.end());

    const uint8_t fpadCi[2] = {0x20, 0x02};  // type 0, variable, CI set
    DlsAssembly assembly;
    QVERIFY2(!parser.processPad(reinterpret_cast<const uint8_t*>(onWire.constData()),
                                static_cast<size_t>(onWire.size()), fpadCi, assembly),
             "an X-PAD longer than the fixed 196-byte buffer must be rejected "
             "even when its announced CI/data length is short");
    QVERIFY(!assembly.hasLabel);

    // The parser must remain usable: a normal, well-formed pad still decodes.
    const QByteArray okText = QByteArrayLiteral("Hi");
    QByteArray okGroup;
    okGroup.append(static_cast<char>(0x40 | 0x20 | (okText.size() - 1)));  // First|Last|len-1
    okGroup.append(static_cast<char>(0xF0));                             // charset 15, seg 0
    okGroup.append(okText);
    const uint16_t okCrc = static_cast<uint16_t>(~eti::crc16ccitt_false(
        reinterpret_cast<const uint8_t*>(okGroup.constData()), okGroup.size()));
    okGroup.append(static_cast<char>((okCrc >> 8) & 0xFF));
    okGroup.append(static_cast<char>(okCrc & 0xFF));  // 6 bytes total
    QByteArray logicalOk;
    logicalOk.append(static_cast<char>((1 << 5) | 2));  // CI: type 2, len index 1 -> 6
    logicalOk.append(static_cast<char>(0x00));          // end marker
    logicalOk.append(okGroup);
    QByteArray wireOk = logicalOk;
    std::reverse(wireOk.begin(), wireOk.end());
    DlsAssembly ok;
    QVERIFY2(parser.processPad(reinterpret_cast<const uint8_t*>(wireOk.constData()),
                               static_cast<size_t>(wireOk.size()), fpadCi, ok),
             "parser must still decode a valid pad after rejecting an oversized one");
    QVERIFY(ok.hasLabel);
    QCOMPARE(ok.dlText, okText);
}

namespace {
// Minimal FIG 0/2 programme service entry with one DAB+ component (ASCTy 0x3F).
QByteArray fig02ServiceOnSubChannel(uint16_t serviceId, uint8_t subChannelId)
{
    QByteArray data;
    data.append(static_cast<char>(0x02));  // ext 2, P/D = 0 (programme)
    data.append(static_cast<char>((serviceId >> 8) & 0x7F));
    data.append(static_cast<char>(serviceId & 0xFF));
    data.append(static_cast<char>(0x01));  // 1 component
    data.append(static_cast<char>(0x3F));  // TMId 0 + ASCTy 0x3F (DAB+)
    data.append(static_cast<char>((subChannelId << 2) | 0x02));  // SubChId + primary
    QByteArray block;
    block.append(static_cast<char>(0x00 | (data.size() & 0x1F)));  // FIG type 0 + length
    block.append(data);
    return block;
}

// Minimal FIG 1/1 programme service label (charset 0) + 16-bit char flag.
QByteArray fig11ServiceLabel(uint16_t serviceId, const QByteArray& labelIn)
{
    QByteArray data;
    data.append(static_cast<char>(0x01));  // charset 0, ext 1
    data.append(static_cast<char>((serviceId >> 8) & 0x7F));
    data.append(static_cast<char>(serviceId & 0xFF));
    QByteArray label = labelIn;
    while (label.size() < 16) label.append(' ');
    label.truncate(16);
    data.append(label);
    data.append(static_cast<char>(0x00));  // character flag hi
    data.append(static_cast<char>(0x00));  // character flag lo
    QByteArray block;
    block.append(static_cast<char>(0x20 | (data.size() & 0x1F)));  // FIG type 1 + length
    block.append(data);
    return block;
}
}  // namespace

// ============================================================================
// T20 finding 2: the DAB+ sub-channel -> service cache must invalidate on the
// analyzer's monotonic service revision, not only on count changes. A FIG 1/1
// label arriving after the cache was built (with an empty label) must be
// reflected in the resolved service label and the Now Playing panel line.
// ============================================================================
void TestGuiDockLayout::testDabPlusServiceCacheRevision()
{
    clearSettingsStore();
    DABAnalyzerWindow w;
    w.show();
    QCoreApplication::processEvents();

    constexpr uint16_t kServiceId = 0x1234;
    constexpr int kSubChannel = 3;

    // 1) FIG 0/2 only: the DAB+ component exists but its label is unknown yet.
    const uint32_t rev0 = w.figServiceRevisionForTest();
    w.feedFicForTest(fig02ServiceOnSubChannel(kServiceId, kSubChannel));
    QVERIFY2(w.figServiceRevisionForTest() != rev0,
             "FIG 0/2 must advance the service revision");
    // Drain the queued servicesUpdated() before any close: the window's
    // queued-slot connections must not fire against a torn-down analyzer.
    QCoreApplication::processEvents();

    // Build the cache now -> under the old count-only invalidation it pinned
    // the (empty) label for the whole session.
    QVERIFY(w.dabPlusSubChannelKnownForTest(kSubChannel));
    QVERIFY2(w.dabPlusServiceLabelForTest(kSubChannel).isEmpty(),
             "label is not known before FIG 1/1 arrives");

    // 2) FIG 1/1 supplies the real label without changing any count.
    const uint32_t rev1 = w.figServiceRevisionForTest();
    w.feedFicForTest(fig11ServiceLabel(kServiceId, QByteArrayLiteral("RROne FM 101")));
    QVERIFY2(w.figServiceRevisionForTest() != rev1,
             "FIG 1/1 must advance the service revision");
    QCoreApplication::processEvents();

    // 3) The cache must refresh: real label, no stale blank.
    QCOMPARE(w.dabPlusServiceLabelForTest(kSubChannel).trimmed(),
             QStringLiteral("RROne FM 101"));

    // 4) The Now Playing panel line must show the resolved service label.
    QVERIFY(w.feedServiceLabeledDlsForTest(kSubChannel, QByteArrayLiteral("Test Song")));
    QVERIFY2(w.nowPlayingStatusForTest().contains(QStringLiteral("RROne FM 101")),
             qPrintable(QString("panel line must show the real service label, got: %1")
                            .arg(w.nowPlayingStatusForTest())));

    w.close();
    QCoreApplication::processEvents();
}

// ============================================================================
// T25: a queued FIG signal (servicesUpdated/subChannelsUpdated/
// ensembleInfoUpdated) posted right before close must NOT be delivered against
// the torn-down AdvancedFIGAnalyzer. Pre-T25, closeEvent() reset m_figAnalyzer
// and then called processEvents(), which delivered the still-queued
// onServicesUpdated() -> getServiceCount() on the freed analyzer (latent
// crash). Mechanism note (F3): QObject::disconnect only stops FUTURE emissions;
// the already-posted queued call is neutralised by the null-guard in
// onServicesUpdated()/onSubChannelsUpdated() (the actual fix). This feeds a FIG
// update, leaves the queued emission pending, closes the window (closeEvent
// path) and then destroys it.
// ============================================================================
void TestGuiDockLayout::testTeardownDropsQueuedFigSignals()
{
    clearSettingsStore();
    auto* w = new DABAnalyzerWindow;
    w->show();
    QCoreApplication::processEvents();

    // FIG 0/2 advances the service revision and makes AdvancedFIGAnalyzer emit
    // servicesUpdated(). The window's connection is Qt::QueuedConnection, so a
    // delivery event is now pending in this thread's event queue — deliberately
    // NOT drained before close.
    w->feedFicForTest(fig02ServiceOnSubChannel(0x1234, 3));
    const uint32_t revisionBeforeClose = w->figServiceRevisionForTest();
    QVERIFY(revisionBeforeClose != 0u);

    // close() -> closeEvent() resets the analyzer and runs processEvents();
    // the stale queued servicesUpdated() must be a no-op (null-guard), so the
    // window survives and its analyzer-dependent hook reports the torn-down
    // (null-analyzer) state instead of dereferencing a freed pointer.
    w->close();
    QCoreApplication::processEvents();
    QCOMPARE(w->figServiceRevisionForTest(), 0u);  // analyzer gone, window alive

    // Destroy with no close event in flight — the destructor's own disconnect
    // + guard path must also be safe.
    delete w;
    QCoreApplication::processEvents();

    // Reaching here means teardown with a pending queued FIG signal completed
    // without a crash; the post-condition is the null-analyzer state asserted
    // above while the window was still alive.
}

// ============================================================================
// T22: an invalid stream URL must be rejected in the UI BEFORE the core is
// touched — one warning, no receiver created, no connect attempt, no retry.
// ============================================================================
void TestGuiDockLayout::testNetworkStreamInvalidUrlGuard()
{
    clearSettingsStore();
    DABAnalyzerWindow w;
    w.show();
    QCoreApplication::processEvents();

    const int warningsBefore = w.streamValidationWarningCountForTest();
    const int attemptsBefore = w.streamConnectAttemptCountForTest();

    // Every unsupported form must be rejected by the shared validator and
    // must NOT reach receiver creation (the reported localhost loop).
    const QStringList invalid = {
        QStringLiteral("localhost"),
        QStringLiteral("zmq+tcp://localhost:9201"),
        QStringLiteral("tcp://192.168.1.100:9200"),
        QStringLiteral("192.168.1.100:9200"),
        QStringLiteral("udp://127.0.0.1:9200"),
        QString(),
        QStringLiteral("239.192.0.1:0"),
        QStringLiteral("239.192.0.1:99999"),
    };

    for (const QString& url : invalid) {
        QVERIFY2(!w.beginStreamConnectionForTest(url), qPrintable(url));
        QCOMPARE(w.streamReceiverCountForTest(), 0);  // no receiver created
    }

    QCOMPARE(w.streamValidationWarningCountForTest(), warningsBefore + invalid.size());
    QCOMPARE(w.streamConnectAttemptCountForTest(), attemptsBefore);  // no attempt

    // The System Messages strip records the rejection (warning path), and the
    // Connect button was never left disabled.
    QVERIFY2(w.streamStatusTextForTest().contains(QStringLiteral("Disconnected")),
             qPrintable(QString("status must stay Disconnected, got: %1")
                            .arg(w.streamStatusTextForTest())));
    QVERIFY2(w.streamConnectButtonEnabledForTest(),
             "Connect must stay enabled after a rejected URL");

    // Quick examples: the two supported multicast forms are selectable; the
    // TCP/ZMQ documentation entries are DISABLED so they cannot be chosen.
    QVERIFY(w.quickExampleComboForTest());
    QComboBox* combo = w.quickExampleComboForTest();
    QVERIFY(combo->count() >= 4);
    QVERIFY(combo->model()->index(0, 0).flags().testFlag(Qt::ItemIsEnabled));
    QVERIFY(combo->model()->index(1, 0).flags().testFlag(Qt::ItemIsEnabled));
    QVERIFY2(!combo->model()->index(2, 0).flags().testFlag(Qt::ItemIsEnabled),
             "tcp:// example must be disabled (not supported)");
    QVERIFY2(!combo->model()->index(3, 0).flags().testFlag(Qt::ItemIsEnabled),
             "zmq+tcp:// example must be disabled (not supported)");

    w.close();
    QCoreApplication::processEvents();
}

// ============================================================================
// T22 review F3a: once a session has connected, error_occurred (quality /
// non-fatal messages) must be log-only — no status repaint, no button churn.
// Before a confirmed connection the same handler still drives the failure UI.
// ============================================================================
void TestGuiDockLayout::testStreamErrorAfterConnectedIsLogOnly()
{
    clearSettingsStore();
    DABAnalyzerWindow w;
    w.show();
    QCoreApplication::processEvents();

    // --- Live session: error is log-only ---
    w.setStreamEverConnectedForTest(true);
    w.setStreamStatusTextForTest(QStringLiteral("SESSION-LIVE"));
    w.setStreamButtonsEnabledForTest(/*connect*/ false, /*disconnect*/ true, /*record*/ true);

    w.simulateStreamErrorForTest(
        QStringLiteral("Latency above threshold: 101us > 100us"));
    QCoreApplication::processEvents();

    QCOMPARE(w.streamStatusTextForTest(), QStringLiteral("SESSION-LIVE"));
    QVERIFY2(!w.streamConnectButtonEnabledForTest(),
             "post-connect error must not re-enable Connect");
    QVERIFY2(w.streamDisconnectButtonEnabledForTest(),
             "post-connect error must not disable Disconnect");
    QVERIFY2(w.streamRecordButtonEnabledForTest(),
             "post-connect error must not disable Record");

    // --- Still establishing: same handler drives the failure UI ---
    w.setStreamEverConnectedForTest(false);
    w.setStreamStatusTextForTest(QStringLiteral("Connecting..."));
    w.setStreamButtonsEnabledForTest(/*connect*/ false, /*disconnect*/ true, /*record*/ false);

    w.simulateStreamErrorForTest(QStringLiteral("Failed to bind to port 9200"));
    QCoreApplication::processEvents();

    QVERIFY(w.streamConnectButtonEnabledForTest());
    QVERIFY(!w.streamDisconnectButtonEnabledForTest());
    QVERIFY(!w.streamRecordButtonEnabledForTest());
    QVERIFY2(w.streamStatusTextForTest().contains(QStringLiteral("Error")),
             qPrintable(QString("pre-connect error must set the red Error status, got: %1")
                            .arg(w.streamStatusTextForTest())));

    w.close();
    QCoreApplication::processEvents();
}

// ============================================================================
// Refinement C: Tab 2 / Tab 3 section titles are meaningful (no Left/Center/
// Right or placeholder text).
// ============================================================================
void TestGuiDockLayout::testTabPagesDescriptiveTitles()
{
    clearSettingsStore();
    DABAnalyzerWindow w;
    w.show();
    QCoreApplication::processEvents();

    auto* mainTabs = w.findChild<QTabWidget*>(QStringLiteral("mainTabs"));
    QVERIFY(mainTabs);
    QCOMPARE(mainTabs->count(), 3);

    const QStringList generic = {QStringLiteral("Left"), QStringLiteral("Center"),
                                 QStringLiteral("Right"), QStringLiteral("—"),
                                 QStringLiteral("-"), QStringLiteral("N/A"),
                                 QStringLiteral("TODO")};

    QStringList seenTitles;
    for (int tab = 1; tab <= 2; ++tab) {  // Tab 2 = FIC-Analyzer, Tab 3 = FIC-Extractor
        QWidget* page = mainTabs->widget(tab);
        QVERIFY2(page, qPrintable(QString("tab page %1 missing").arg(tab)));
        const QList<QGroupBox*> groups = page->findChildren<QGroupBox*>();
        QVERIFY2(!groups.isEmpty(),
                 qPrintable(QString("tab %1 must have titled sections").arg(tab)));
        for (QGroupBox* g : groups) {
            const QString title = g->title().trimmed();
            QVERIFY2(!title.isEmpty(), "section title must not be empty");
            QVERIFY2(!generic.contains(title, Qt::CaseInsensitive),
                     qPrintable(QString("generic section title '(%1)' in tab %2")
                                    .arg(title).arg(tab)));
            seenTitles << title;
        }
    }

    // The meaningful names requested by the user must be present.
    const QStringList expected = {
        QStringLiteral("Overview"), QStringLiteral("Service Tree"),
        QStringLiteral("Digital Org Info"), QStringLiteral("Service Components"),
        QStringLiteral("Ensemble Table"), QStringLiteral("Sub-Channel Table"),
        QStringLiteral("Service Table"), QStringLiteral("FIG Content"),
        QStringLiteral("FIG Instance List"), QStringLiteral("FIG Item Details"),
    };
    for (const QString& name : expected) {
        QVERIFY2(seenTitles.contains(name),
                 qPrintable(QString("expected section title missing: %1 (have: %2)")
                                .arg(name, seenTitles.join(", "))));
    }

    w.close();
    QCoreApplication::processEvents();
}

// ============================================================================
// MOT SlideShow (option A): X-PAD CI 12/13 -> adapter -> MOTProtocol -> panel
// ============================================================================
namespace {

// 7-byte standard MOT header core: body size(28) header size(13) type(6) sub(9).
QByteArray motStdHeaderCore(uint32_t bodySize, uint8_t contentType, uint16_t subType)
{
    QByteArray h(7, '\0');
    auto* d = reinterpret_cast<uint8_t*>(h.data());
    d[0] = static_cast<uint8_t>((bodySize >> 20) & 0xFF);
    d[1] = static_cast<uint8_t>((bodySize >> 12) & 0xFF);
    d[2] = static_cast<uint8_t>((bodySize >> 4) & 0xFF);
    d[3] = static_cast<uint8_t>(((bodySize & 0x0F) << 4) | ((7u >> 9) & 0x0F));
    d[4] = static_cast<uint8_t>((7u >> 1) & 0xFF);
    d[5] = static_cast<uint8_t>(((7u & 0x01) << 7) |
                                ((contentType & 0x3F) << 1) |
                                ((subType >> 8) & 0x01));
    d[6] = static_cast<uint8_t>(subType & 0xFF);
    return h;
}

// Standard MOT header entity: core + one or more text extension parameters.
// Each parameter is encoded per EN 301 234 §6.2 Fig.22 with PLI=11 (7-/15-bit
// DataFieldLength indicator) and a leading charset/Rfa byte (charset 0x0 =
// EBU Latin, per §6.2.2.1.1 Fig.24), with the 13-bit header size updated.
QByteArray motStdHeaderEntity(uint32_t bodySize, uint8_t contentType, uint16_t subType,
                              const QList<QPair<uint8_t, QByteArray>>& textParams)
{
    QByteArray h = motStdHeaderCore(bodySize, contentType, subType);
    for (const auto& p : textParams) {
        QByteArray data;
        data.append(static_cast<char>(0x00));  // charset 0x0 (EBU Latin) + Rfa 0x0
        data.append(p.second);

        h.append(static_cast<char>(0xC0 | (p.first & 0x3F)));  // PLI=11 + ParamId
        const int n = data.size();
        if (n < 0x80) {
            h.append(static_cast<char>(n & 0x7F));
        } else {
            h.append(static_cast<char>(0x80 | ((n >> 8) & 0x7F)));
            h.append(static_cast<char>(n & 0xFF));
        }
        h.append(data);
    }
    const uint16_t headerSize = static_cast<uint16_t>(h.size());
    auto* d = reinterpret_cast<uint8_t*>(h.data());
    d[3] = static_cast<uint8_t>((d[3] & 0xF0) | ((headerSize >> 9) & 0x0F));
    d[4] = static_cast<uint8_t>((headerSize >> 1) & 0xFF);
    d[5] = static_cast<uint8_t>((d[5] & 0x7F) | ((headerSize & 0x01) << 7));
    return h;
}

// Standard MOT MSC data group (EN 300 401 §5.3.3): header + MOT session
// header + segmentation header + payload + ~CRC-16/CCITT-FALSE.
QByteArray motStdDataGroup(uint8_t dgType, uint16_t tid, uint16_t seg, bool last,
                           const QByteArray& payload)
{
    QByteArray dg;
    dg.append(static_cast<char>(0x40 | 0x20 | 0x10 | (dgType & 0x0F)));
    dg.append(static_cast<char>(0x00));
    dg.append(static_cast<char>((last ? 0x80 : 0x00) | ((seg >> 8) & 0x7F)));
    dg.append(static_cast<char>(seg & 0xFF));
    dg.append(static_cast<char>(0x10 | 0x02));  // transport id flag, len = 2
    dg.append(static_cast<char>((tid >> 8) & 0xFF));
    dg.append(static_cast<char>(tid & 0xFF));
    dg.append(static_cast<char>((payload.size() >> 8) & 0x1F));
    dg.append(static_cast<char>(payload.size() & 0xFF));
    dg.append(payload);
    const uint16_t crc = static_cast<uint16_t>(~eti::crc16ccitt_false(
        reinterpret_cast<const uint8_t*>(dg.constData()), dg.size()));
    dg.append(static_cast<char>((crc >> 8) & 0xFF));
    dg.append(static_cast<char>(crc & 0xFF));
    return dg;
}

// Wrap one X-PAD variable-size sub-field in on-wire (reversed) byte order.
QByteArray motOnWireSubfield(int ciType, const QByteArray& chunk)
{
    static const int lens[8] = {4, 6, 8, 12, 16, 24, 32, 48};
    int idx = -1;
    for (int i = 0; i < 8; ++i) {
        if (lens[i] >= chunk.size()) {
            idx = i;
            break;
        }
    }
    Q_ASSERT(idx >= 0);
    QByteArray logical;
    logical.append(static_cast<char>((idx << 5) | (ciType & 0x1F)));
    logical.append(static_cast<char>(0x00));  // CI end marker
    QByteArray padded = chunk;
    padded.resize(lens[idx], '\0');
    logical.append(padded);
    std::reverse(logical.begin(), logical.end());
    return logical;
}

// Full on-wire X-PAD sequence for one standard data group: DGLI (CI 1) +
// MOT start (CI 12) + MOT continuations (CI 13), <=48-byte sub-fields.
QList<QByteArray> motOnWireDataGroup(const QByteArray& dg)
{
    QList<QByteArray> out;
    QByteArray dgli;
    dgli.append(static_cast<char>((dg.size() >> 8) & 0x3F));
    dgli.append(static_cast<char>(dg.size() & 0xFF));
    const uint16_t c = static_cast<uint16_t>(~eti::crc16ccitt_false(
        reinterpret_cast<const uint8_t*>(dgli.constData()), 2));
    dgli.append(static_cast<char>((c >> 8) & 0xFF));
    dgli.append(static_cast<char>(c & 0xFF));
    out.append(motOnWireSubfield(1, dgli));

    out.append(motOnWireSubfield(12, dg.left(48)));
    for (int off = 48; off < dg.size(); off += 48) {
        out.append(motOnWireSubfield(13, dg.mid(off, 48)));
    }
    return out;
}

// Build one variable-size X-PAD holding MULTIPLE CI sub-fields (CI bytes first,
// then the end marker, then the concatenated, length-padded sub-field data), in
// on-wire (reversed) byte order.
QByteArray motOnWireMultiCi(const QList<QPair<int, QByteArray>>& subfields)
{
    static const int lens[8] = {4, 6, 8, 12, 16, 24, 32, 48};
    QByteArray logical;
    for (const auto& s : subfields) {
        int idx = -1;
        for (int i = 0; i < 8; ++i) {
            if (lens[i] >= s.second.size()) {
                idx = i;
                break;
            }
        }
        Q_ASSERT(idx >= 0);
        logical.append(static_cast<char>((idx << 5) | (s.first & 0x1F)));
    }
    logical.append(static_cast<char>(0x00));  // CI end marker
    for (const auto& s : subfields) {
        int idx = -1;
        for (int i = 0; i < 8; ++i) {
            if (lens[i] >= s.second.size()) {
                idx = i;
                break;
            }
        }
        QByteArray padded = s.second;
        padded.resize(lens[idx], '\0');
        logical.append(padded);
    }
    std::reverse(logical.begin(), logical.end());
    return logical;
}

// A raw ci_flag==0 continuation X-PAD: no CI bytes, just the data (reversed).
QByteArray motOnWireContinuation(const QByteArray& data)
{
    QByteArray wire = data;
    std::reverse(wire.begin(), wire.end());
    return wire;
}

// A small valid PNG (Qt always ships the PNG codec).
QByteArray motTinyPng()
{
    QImage img(8, 8, QImage::Format_RGB32);
    img.fill(Qt::red);
    QByteArray ba;
    QBuffer buf(&ba);
    buf.open(QIODevice::WriteOnly);
    img.save(&buf, "PNG");
    return ba;
}

}  // namespace

void TestGuiDockLayout::testMotPadAdapterXpadPipeline()
{
    qRegisterMetaType<eti::mot::MOTObject>("eti::mot::MOTObject");

    // Adapter unit test: synthetic MOT header+body X-PAD data groups (CI 12/13)
    // must reassemble and fire objectComplete with the expected transport id and
    // body bytes. The DGLI (CI 1) announces each standard data group's length.
    DabPlusSubchannelParser parser;
    eti::mot::MOTProtocol mot;
    eti::mot::MotPadAdapter adapter(&mot);
    QSignalSpy spy(&mot, &eti::mot::MOTProtocol::objectComplete);
    QVERIFY(spy.isValid());
    parser.onMotDataGroup = [&adapter](const QByteArray& dg) {
        adapter.processDataGroup(0x2C61u, dg);
    };

    const uint16_t tid = 0x0123;
    const QByteArray body(217, 'Z');
    const QByteArray header = motStdHeaderCore(body.size(), 0x02, 0x01);

    const auto feed = [&parser](const QByteArray& dg) {
        const QList<QByteArray> wires = motOnWireDataGroup(dg);
        for (const QByteArray& w : wires) {
            const uint8_t fpad[2] = {0x20, 0x02};  // type 0, variable X-PAD, CI set
            DlsAssembly a;
            parser.processPad(reinterpret_cast<const uint8_t*>(w.constData()),
                              static_cast<size_t>(w.size()), fpad, a);
        }
    };

    feed(motStdDataGroup(3, tid, 0, true, header));
    QCOMPARE(spy.count(), 0);  // header alone must not complete an object
    feed(motStdDataGroup(4, tid, 0, true, body));

    QCOMPARE(spy.count(), 1);
    const QList<QVariant> args = spy.takeFirst();
    QCOMPARE(args.at(0).toUInt(), static_cast<uint>(tid));
    const eti::mot::MOTObject object = qvariant_cast<eti::mot::MOTObject>(args.at(1));
    const QByteArray got(reinterpret_cast<const char*>(object.body.data()),
                         static_cast<int>(object.body.size()));
    QCOMPARE(got, body);

    const eti::mot::MotPadAdapter::Statistics st = adapter.statistics();
    QCOMPARE(st.dataGroups, static_cast<uint64_t>(2));
    QCOMPARE(st.headerSegments, static_cast<uint64_t>(1));
    QCOMPARE(st.bodySegments, static_cast<uint64_t>(1));
    QCOMPARE(st.objectsCompleted, static_cast<uint64_t>(1));
    QCOMPARE(st.objectsEmitted, static_cast<uint64_t>(1));
}

void TestGuiDockLayout::testMotXpadCiFlagZeroContinuation()
{
    // ci_flag==0 AU continuation: verify the tracked continuation length is the
    // MOT sub-field length, not the whole announced X-PAD (which includes a
    // co-resident DGLI), and that a group split across such AUs reassembles.
    DabPlusSubchannelParser parser;
    eti::mot::MOTProtocol mot;
    eti::mot::MotPadAdapter adapter(&mot);
    QSignalSpy spy(&mot, &eti::mot::MOTProtocol::objectComplete);
    parser.onMotDataGroup = [&adapter](const QByteArray& dg) {
        adapter.processDataGroup(0x2C61u, dg);
    };

    const uint16_t tid = 0x0123;
    const QByteArray body(120, 'Q');
    const QByteArray header = motStdHeaderCore(body.size(), 0x02, 0x01);
    const QByteArray headerDg = motStdDataGroup(3, tid, 0, true, header);
    const QByteArray bodyDg = motStdDataGroup(4, tid, 0, true, body);

    const auto dgliFor = [](const QByteArray& dg) {
        QByteArray dgli;
        dgli.append(static_cast<char>((dg.size() >> 8) & 0x3F));
        dgli.append(static_cast<char>(dg.size() & 0xFF));
        const uint16_t c = static_cast<uint16_t>(~eti::crc16ccitt_false(
            reinterpret_cast<const uint8_t*>(dgli.constData()), 2));
        dgli.append(static_cast<char>((c >> 8) & 0xFF));
        dgli.append(static_cast<char>(c & 0xFF));
        return dgli;
    };
    const auto feedCi = [&parser](const QByteArray& wire) {
        const uint8_t fpad[2] = {0x20, 0x02};  // type 0, variable X-PAD, CI set
        DlsAssembly a;
        parser.processPad(reinterpret_cast<const uint8_t*>(wire.constData()),
                          static_cast<size_t>(wire.size()), fpad, a);
    };
    const auto feedCont = [&parser](const QByteArray& wire) {
        const uint8_t fpad[2] = {0x20, 0x00};  // type 0, variable X-PAD, CI clear
        DlsAssembly a;
        parser.processPad(reinterpret_cast<const uint8_t*>(wire.constData()),
                          static_cast<size_t>(wire.size()), fpad, a);
    };

    // AU1: DGLI (CI 1) + CI 12 start co-resident in one X-PAD (header DG).
    feedCi(motOnWireMultiCi({{1, dgliFor(headerDg)}, {12, headerDg}}));
    QCOMPARE(spy.count(), 0);

    // AU2: DGLI + CI 12 carrying the first 48 bytes of the body DG. The tracked
    // continuation length must be the CI-12 sub-field length (48), NOT the whole
    // announced X-PAD (~2 CI + 4 DGLI + 48 = 54).
    const QByteArray firstChunk = bodyDg.left(48);
    feedCi(motOnWireMultiCi({{1, dgliFor(bodyDg)}, {12, firstChunk}}));
    QCOMPARE(parser.motContinuationLen, static_cast<int>(firstChunk.size()));

    // AU3/AU4: ci_flag==0 continuations carrying the remaining body DG bytes.
    feedCont(motOnWireContinuation(bodyDg.mid(48, 48)));
    QCOMPARE(spy.count(), 0);
    feedCont(motOnWireContinuation(bodyDg.mid(96)));

    QCOMPARE(spy.count(), 1);
    const eti::mot::MOTObject object =
        qvariant_cast<eti::mot::MOTObject>(spy.takeFirst().at(1));
    const QByteArray got(reinterpret_cast<const char*>(object.body.data()),
                         static_cast<int>(object.body.size()));
    QCOMPARE(got, body);
}

void TestGuiDockLayout::testMotSlideshowPanelAndTabs()
{
    clearSettingsStore();
    DABAnalyzerWindow w;
    w.show();
    QCoreApplication::processEvents();

    // Dock title + inner tabs (option A).
    auto* m1 = w.findChild<ads::CDockManager*>("tab1DockManager");
    QVERIFY(m1);
    ads::CDockWidget* np = m1->findDockWidget(QStringLiteral("tab1_NowPlaying"));
    QVERIFY(np);
    QCOMPARE(np->windowTitle(), QStringLiteral("Now Playing / Slideshow"));
    QVERIFY(np->dockAreaWidget());
    QVERIFY2(np->dockAreaWidget()->minimumHeight() >= 200,
             qPrintable(QString("Now Playing dock must grow to >=200px, got %1")
                            .arg(np->dockAreaWidget()->minimumHeight())));

    QTabWidget* tabs = w.nowPlayingTabsForTest();
    QVERIFY(tabs);
    QCOMPARE(tabs->count(), 2);
    QCOMPARE(tabs->tabText(0), QStringLiteral("DLS+ Text"));
    QCOMPARE(tabs->tabText(1), QStringLiteral("Slideshow"));

    // Documented initial slideshow state.
    QCOMPARE(w.slideshowStatusForTest(), QStringLiteral("Waiting for slideshow…"));
    QVERIFY(w.slideshowImageForTest().isNull());
    QCOMPARE(w.motObjectsForTest(), 0);

    // --- Progress state + completion through the real X-PAD 12/13 path -----
    const QByteArray png = motTinyPng();
    QVERIFY2(!png.isEmpty(), "test PNG must encode");
    const uint16_t tid = 0x0201;
    const QByteArray header = motStdHeaderEntity(
        png.size(), 0x02, 0x03,
        {{0x0C, QByteArray("panel-slide.png")}});  // image/PNG + content name
    const int cut = png.size() - 10;
    QVERIFY(cut > 0);

    // Header + first body segment (not last) -> live progress.
    QList<QByteArray> wires = motOnWireDataGroup(motStdDataGroup(3, tid, 0, true, header));
    wires += motOnWireDataGroup(motStdDataGroup(4, tid, 0, false, png.left(cut)));
    QVERIFY(w.feedMotXpadsForTest(wires));
    QVERIFY2(w.slideshowStatusForTest().contains(QStringLiteral("Receiving")),
             qPrintable(QString("expected Receiving state, got: %1")
                            .arg(w.slideshowStatusForTest())));
    QCOMPARE(w.motImagesDecodedForTest(), 0);

    // Final body segment -> decode + show.
    QVERIFY(w.feedMotXpadsForTest(
        motOnWireDataGroup(motStdDataGroup(4, tid, 1, true, png.mid(cut)))));

    QCOMPARE(w.motObjectsForTest(), 1);
    QCOMPARE(w.motImagesDecodedForTest(), 1);
    QVERIFY(!w.slideshowImageForTest().isNull());
    QVERIFY(w.slideshowImageForTest().width() > 0);
    QVERIFY2(w.slideshowStatusForTest().contains(QStringLiteral("Showing slide")),
             qPrintable(QString("expected Showing-slide state, got: %1")
                            .arg(w.slideshowStatusForTest())));
    QVERIFY2(w.slideshowInfoForTest().contains(QStringLiteral("TID")),
             qPrintable(QString("info line must carry the transport id, got: %1")
                            .arg(w.slideshowInfoForTest())));
    // T26: the standard MOT header extension parameter (content name) must have
    // survived the parse and be rendered in the info line (no reframing).
    QVERIFY2(w.slideshowInfoForTest().contains(QStringLiteral("panel-slide.png")),
             qPrintable(QString("info line must carry the restored content name, got: %1")
                            .arg(w.slideshowInfoForTest())));
    QCOMPARE(w.motHeaderParamsSeenForTest(), 1);
    QCOMPARE(w.lastMotContentNameForTest(), QStringLiteral("panel-slide.png"));
    // Bounded retention.
    QVERIFY(w.motSlideHistorySizeForTest() <= 8);

    w.close();
    QCoreApplication::processEvents();
}

void TestGuiDockLayout::testMotSlideshowRealFixture()
{
    clearSettingsStore();
    DABAnalyzerWindow w;
    w.show();
    QCoreApplication::processEvents();

    const QString fixture = QStringLiteral("../../eti/bkk_20062022_141637.eti");
    QVERIFY2(QFileInfo::exists(fixture),
             qPrintable(QString("fixture missing: %1 (cwd=%2)")
                            .arg(fixture, QDir::currentPath())));
    w.autoLoadFile(fixture);
    QCoreApplication::processEvents();
    QTRY_VERIFY_WITH_TIMEOUT(w.isFileLoaded() && w.analyzerFigInstanceCount() > 0, 120000);

    // The Bangkok capture really carries MOT SlideShow (X-PAD CI 12/13): the
    // whole pipeline must have decoded the documented number of objects/images.
    const eti::mot::MotPadAdapter::Statistics ms = w.motAdapterStatisticsForTest();

    // Fixture parity (T26): the standard-layout parser reproduces the
    // re-framing-era counters exactly (2773 DGs = 112 header + 2661 body,
    // 97 objects → 97 images, 0 CRC errors, 0 MOT errors).
    QCOMPARE(static_cast<quint64>(ms.dataGroups), static_cast<quint64>(2773));
    QCOMPARE(static_cast<quint64>(ms.crcErrors), static_cast<quint64>(0));
    QCOMPARE(static_cast<quint64>(ms.headerSegments), static_cast<quint64>(112));
    QCOMPARE(static_cast<quint64>(ms.bodySegments), static_cast<quint64>(2661));
    QCOMPARE(static_cast<quint64>(ms.objectsCompleted), static_cast<quint64>(97));
    QCOMPARE(static_cast<quint64>(ms.objectsEmitted), static_cast<quint64>(97));
    QCOMPARE(w.motObjectsForTest(), 97);
    QCOMPARE(w.motImagesDecodedForTest(), 97);
    QCOMPARE(w.motErrorsForTest(), 0);  // zero MOT warnings/errors on the capture

    // F2/T26: the Bangkok capture's MOT headers DO carry extension parameters
    // (ContentName + TriggerTime, 22..43-byte headers). They must be recovered,
    // not silently dropped by the old/incorrect PLI model.
    QVERIFY2(w.motHeaderParamsSeenForTest() >= 1,
             "the fixture MOT headers must expose extension parameters");
    QVERIFY2(!w.lastMotContentNameForTest().isEmpty(),
             "at least one fixture object must expose a non-empty content name");
    QVERIFY2(w.lastMotContentNameForTest().endsWith(QStringLiteral(".jpg")),
             qPrintable(QString("expected a JPEG filename, got: %1")
                            .arg(w.lastMotContentNameForTest())));

    QVERIFY2(w.motObjectsForTest() >= 1,
             "the fixture carries MOT objects; the adapter must complete >= 1");
    QVERIFY2(w.motImagesDecodedForTest() >= 1,
             "at least one MOT body must decode to a QImage");
    const QImage slide = w.slideshowImageForTest();
    QVERIFY2(!slide.isNull(), "the slideshow image must be non-null after the load");
    QVERIFY(slide.width() > 0 && slide.height() > 0);
    QVERIFY2(w.motSlideHistorySizeForTest() <= 8,
             "retained slideshow images must stay bounded");
    QVERIFY2(w.slideshowStatusForTest().contains(QStringLiteral("Showing slide")),
             qPrintable(QString("slideshow status after load: %1")
                            .arg(w.slideshowStatusForTest())));

    // DLS+ side must be unchanged by the MOT wave (the parser feeds both).
    QVERIFY2(w.dlsPlusMessageCountForTest() > 0, "DLS+ labels must still decode");
    QCOMPARE(w.dlsPlusMessageCountForTest(), 1114);  // unchanged by T26

    w.close();
    QCoreApplication::processEvents();
}

QTEST_MAIN(TestGuiDockLayout)
#include "test_gui_dock_layout.moc"