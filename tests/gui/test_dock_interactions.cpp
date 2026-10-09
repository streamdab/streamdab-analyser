/**
 * @file test_dock_interactions.cpp
 * @brief Automated Qt-ADS dock INTERACTION QA for DABAnalyserWindow (v1.3, T4)
 *
 * Companion to test_gui_dock_layout.cpp. That suite validates the DEFAULT
 * layout + persistence + version reset; THIS suite drives the interactive dock
 * operations a user performs with the mouse and asserts the resulting state is
 * sane and survives a restart:
 *
 *   1. Float -> re-dock: setFloating() then re-dock into the ORIGINAL area;
 *      the dock is managed again, visible, and its area membership is intact.
 *      Covered per dock manager (Tab-1 tabbed group + bottom strip).
 *   2. Auto-hide toggle -> restore: setAutoHide(true) puts the dock in an
 *      auto-hide container with a side-bar rail tab; toggling back leaves the
 *      dock visible, docked and findable (no lost dock). Tab-1 + bottom.
 *   3. Move across areas: left -> right (Tab-1, verified via the root splitter
 *      column) and bottom -> center (bottom manager), then moved back.
 *   4. Tabify / untabify: addDockWidgetTabToArea() merges two standalone docks
 *      into one area (2 tabs); moving one out yields two areas again.
 *   5. Persistence after interactions: a non-default arrangement (one dock
 *      floated + one moved + one detached from its group) is saved on close()
 *      and restored by a second window (floating + area membership).
 *   6. Reachability: after all interactions + a restart every one of the 27
 *      docks is still findable by objectName and toggleable (toggleView
 *      true/false) with no crash and no lost dock.
 *
 * Compiles the REAL application: src/main.cpp is included with GUI_TEST_MODE
 * (which excludes main()); the rest of ${SOURCES} is compiled by the target.
 * All assertions run offscreen (QT_QPA_PLATFORM=offscreen), no modal dialogs.
 *
 * @date October 2026
 */

#include <QtTest/QtTest>
#include <QAccessible>
#include <QStandardPaths>
#include <QSettings>
#include <QSplitter>
#include <QSet>
#include <QStringList>

#include "DockManager.h"
#include "DockWidget.h"
#include "DockAreaWidget.h"
#include "AutoHideDockContainer.h"
#include "AutoHideTab.h"

// Include the real application window implementation (main() excluded via
// GUI_TEST_MODE compile definition). DABAnalyserWindow + all panels come here.
#include "../src/main.cpp"

// Headless platform detection (copied from main.cpp)
static bool isHeadlessPlatform()
{
    const QString platform = QApplication::platformName();
    return platform == QLatin1String("offscreen") ||
           platform == QLatin1String("minimal");
}

class TestDockInteractions : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanupTestCase();
    void init();
    void cleanup();

    // 1. Float -> re-dock (per manager)
    void testFloatAndRedockTab1();
    void testFloatAndRedockBottom();
    // 2. Auto-hide toggle -> restore (per manager)
    void testAutoHideToggleTab1();
    void testAutoHideToggleBottom();
    // 3. Move across areas
    void testMoveAcrossAreasTab1();
    void testMoveAcrossAreasBottom();
    // 4. Tabify / untabify
    void testTabifyAndUntabify();
    // 5. Persistence after a non-default arrangement
    void testPersistenceAfterInteractions();
    // 6. Every dock still reachable + toggleable after interactions + restart
    void testAllDocksReachableAfterInteractionsAndRestart();

private:
    // The full expected dock inventory (31). Kept in sync with
    // test_gui_dock_layout.cpp::testDocksWithExpectedNames. Phase 2A added 3 data-service docks; Phase 2C added tab2_AdvancedFig.
    static QStringList expectedDockNames()
    {
        return {
            // Tab 1 (18; Phase 2A: +3 data-service docks)
            QStringLiteral("tab1_Player"), QStringLiteral("tab1_Decoder"),
            QStringLiteral("tab1_Audio"), QStringLiteral("tab1_System"),
            QStringLiteral("tab1_EnsembleExplorer"), QStringLiteral("tab1_SubchannelOrg"),
            QStringLiteral("tab1_CUUsage"),
            QStringLiteral("tab1_FrameNavigator"), QStringLiteral("tab1_HexCompliance"),
            QStringLiteral("tab1_NowPlaying"),
            QStringLiteral("tab1_ETIOverview"), QStringLiteral("tab1_Status"),
            QStringLiteral("tab1_Timing"),
            QStringLiteral("tab1_ErrorCounter"), QStringLiteral("tab1_StreamStats"),
            QStringLiteral("tab1_EPG"), QStringLiteral("tab1_Journaline"),
            QStringLiteral("tab1_TPEG"),
            // Tab 2 (4; Phase 2C: +1 Advanced FIG Analyser dock)
            QStringLiteral("tab2_Left"), QStringLiteral("tab2_Center"),
            QStringLiteral("tab2_Right"), QStringLiteral("tab2_AdvancedFig"),
            // Tab 3 (2)
            QStringLiteral("tab3_Left"), QStringLiteral("tab3_Right"),
            // Shared bottom (7; T41 removed the redundant legacy Messages tab)
            QStringLiteral("bottom_SystemMessages"), QStringLiteral("bottom_Performance"),
            QStringLiteral("bottom_RealTimeChart"),
            QStringLiteral("bottom_ErrorDetection"), QStringLiteral("bottom_EtsiCompliance"),
            QStringLiteral("bottom_Logging"),
            QStringLiteral("bottom_Constellation"),
        };
    }
    // Removes every DABAnalyserWindow QSettings key so each window-based test
    // starts from a CLEARED store (window.close() saves dock state; without
    // this the next window would restore the previous layout).
    static void clearSettingsStore()
    {
        QSettings s(QStringLiteral("StreamDAB-Analyser"), QStringLiteral("DABAnalyser"));
        s.remove(QStringLiteral("window/geometry"));
        s.remove(QStringLiteral("docking/layout_version"));
        s.remove(QStringLiteral("docking/tab1"));
        s.remove(QStringLiteral("docking/tab2"));
        s.remove(QStringLiteral("docking/tab3"));
        s.remove(QStringLiteral("docking/bottom"));
        s.sync();
    }

    static void pump()
    {
        QCoreApplication::processEvents();
    }

    // Root splitter of a manager, derived from a dock that belongs to it.
    static QSplitter* rootOfDock(ads::CDockManager* m, ads::CDockWidget* d)
    {
        QWidget* w = d->dockAreaWidget();
        while (w && w->parentWidget() != m) {
            w = w->parentWidget();
        }
        return qobject_cast<QSplitter*>(w);
    }

    // Column index of an area inside the manager's top-level splitter.
    static int columnIndexOf(QSplitter* root, QWidget* area)
    {
        QWidget* w = area;
        while (w && w->parentWidget() != root) {
            w = w->parentWidget();
        }
        return w ? root->indexOf(w) : -1;
    }

    // Every dock the manager still tracks (dockWidgetsMap is the registry that
    // save/restore and findDockWidget rely on).
    static QSet<QString> dockNames(const ads::CDockManager* m)
    {
        QSet<QString> names;
        const QMap<QString, ads::CDockWidget*> map = m->dockWidgetsMap();
        for (auto it = map.cbegin(); it != map.cend(); ++it) {
            names.insert(it.key());
        }
        return names;
    }

    static int totalDockCount(const DABAnalyserWindow& w)
    {
        int total = 0;
        const QList<ads::CDockManager*> managers = w.findChildren<ads::CDockManager*>();
        for (const ads::CDockManager* m : managers) {
            total += m->dockWidgetsMap().size();
        }
        return total;
    }
};

void TestDockInteractions::initTestCase()
{
    // These tests build several top-level DABAnalyserWindow objects one after
    // another. With Qt accessibility active (a UI Automation client on the
    // Windows CI runner), QWidget::setWindowTitle() in the NEXT window's
    // constructor can hit a stale accessible-interface cache entry left by a
    // destroyed window at the same address (QAccessibleWidget::text() ->
    // QWidget::accessibleName() with a null widget -> access violation). No
    // assistive technology is needed here, so keep accessibility off.
    if (QAccessible::isActive()) {
        qInfo() << "QAccessible was active at test start; deactivating";
        QAccessible::setActive(false);
    }
    // Redirect QSettings to Qt's test-mode location so the real user config is
    // never touched, and start empty. SettingsDialog uses default QSettings, so
    // match the app's org/app names too.
    QStandardPaths::setTestModeEnabled(true);
    QCoreApplication::setOrganizationName(QStringLiteral("StreamDAB-Analyser"));
    QCoreApplication::setApplicationName(QStringLiteral("DABAnalyser"));
    clearSettingsStore();
}

void TestDockInteractions::cleanupTestCase()
{
    clearSettingsStore();
}

void TestDockInteractions::init()
{
    // Per-test isolation: each interactive scenario starts from a fresh default
    // layout (no state leaking from a previous interaction test).
    clearSettingsStore();
}

void TestDockInteractions::cleanup()
{
    clearSettingsStore();
}

// ============================================================================
// 1. FLOAT -> RE-DOCK
// ============================================================================
void TestDockInteractions::testFloatAndRedockTab1()
{
    if (isHeadlessPlatform()) {
        QSKIP("Floating docks not supported in headless/offscreen mode");
        return;
    }

    DABAnalyserWindow w;
    w.show();
    pump();

    auto* m1 = w.findChild<ads::CDockManager*>("tab1DockManager");
    QVERIFY(m1);

    // A dock inside a TABBED group: floating it must not destroy the group
    // area, so we can re-dock into the ORIGINAL area.
    ads::CDockWidget* dock = m1->findDockWidget(QStringLiteral("tab1_CUUsage"));
    QVERIFY(dock);
    ads::CDockAreaWidget* originalArea = dock->dockAreaWidget();
    QVERIFY(originalArea);
    const int originalCount = originalArea->dockWidgetsCount();
    QVERIFY(originalCount >= 3);  // Signal Analysis group (Ensemble|Subchannel|CUUsage)

    // Detach into a floating window.
    dock->setFloating();
    pump();
    QVERIFY2(dock->isFloating() || dock->isInFloatingContainer(),
             "CUUsage must be floating after setFloating()");
    QVERIFY2(dock->isInFloatingContainer(),
             "CUUsage must be in a floating container");

    // Re-dock into the original tabbed area.
    m1->addDockWidgetTabToArea(dock, originalArea);
    dock->setAsCurrentTab();
    pump();

    QVERIFY2(!dock->isInFloatingContainer(), "CUUsage must be docked again");
    QVERIFY2(!dock->isClosed(), "re-docked CUUsage must not be closed");
    QCOMPARE(dock->dockAreaWidget(), originalArea);
    QVERIFY2(originalArea->dockWidgets().contains(dock),
             "original area must list CUUsage again");
    QCOMPARE(originalArea->dockWidgetsCount(), originalCount);
    QVERIFY2(dock->isVisible(), "re-docked CUUsage must be visible");
    // Still registered in the manager.
    QCOMPARE(m1->findDockWidget(QStringLiteral("tab1_CUUsage")), dock);

    w.close();
    pump();
}

void TestDockInteractions::testFloatAndRedockBottom()
{
    if (isHeadlessPlatform()) {
        QSKIP("Floating docks not supported in headless/offscreen mode");
        return;
    }

    DABAnalyserWindow w;
    w.show();
    pump();

    auto* mBottom = w.findChild<ads::CDockManager*>("bottomDockManager");
    QVERIFY(mBottom);

    ads::CDockWidget* dock = mBottom->findDockWidget(QStringLiteral("bottom_Performance"));
    QVERIFY(dock);
    ads::CDockAreaWidget* originalArea = dock->dockAreaWidget();
    QVERIFY(originalArea);
    const int originalCount = originalArea->dockWidgetsCount();
    QVERIFY(originalCount >= 7);  // 7-tab shared bottom strip (T41) 

    dock->setFloating();
    pump();
    QVERIFY2(dock->isInFloatingContainer(),
             "bottom Performance must be in a floating container");

    mBottom->addDockWidgetTabToArea(dock, originalArea);
    dock->setAsCurrentTab();
    pump();

    QVERIFY2(!dock->isInFloatingContainer(), "Performance must be docked again");
    QVERIFY2(!dock->isClosed(), "re-docked Performance must not be closed");
    QCOMPARE(dock->dockAreaWidget(), originalArea);
    QVERIFY(originalArea->dockWidgets().contains(dock));
    QCOMPARE(originalArea->dockWidgetsCount(), originalCount);
    QVERIFY2(dock->isVisible(), "re-docked Performance must be visible");
    QCOMPARE(mBottom->findDockWidget(QStringLiteral("bottom_Performance")), dock);

    w.close();
    pump();
}

// ============================================================================
// 2. AUTO-HIDE TOGGLE -> RESTORE
// ============================================================================
void TestDockInteractions::testAutoHideToggleTab1()
{
    DABAnalyserWindow w;
    w.show();
    pump();

    auto* m1 = w.findChild<ads::CDockManager*>("tab1DockManager");
    QVERIFY(m1);

    // Standalone dock (Now Playing) in the center column.
    ads::CDockWidget* dock = m1->findDockWidget(QStringLiteral("tab1_NowPlaying"));
    QVERIFY(dock);
    QVERIFY(!dock->isAutoHide());

    // Pin to auto-hide: a rail tab must appear in a side bar.
    dock->setAutoHide(true);
    pump();
    QVERIFY2(dock->isAutoHide(), "Now Playing must report auto-hide after pinning");
    QVERIFY2(dock->autoHideDockContainer() != nullptr,
             "auto-hidden dock must have an auto-hide container");
    QVERIFY2(dock->sideTabWidget() != nullptr,
             "auto-hidden dock must have a side-bar rail tab (title-bar/rail state)");
    QVERIFY2(dock->autoHideLocation() != ads::SideBarNone,
             "auto-hidden dock must report a real side-bar location");
    QCOMPARE(m1->findDockWidget(QStringLiteral("tab1_NowPlaying")), dock);

    // Un-pin: must come back visible and docked (no lost dock).
    dock->setAutoHide(false);
    QTRY_VERIFY_WITH_TIMEOUT(!dock->isAutoHide(), 5000);
    pump();
    QVERIFY2(dock->autoHideDockContainer() == nullptr,
             "auto-hide container must be gone after un-pinning");
    QVERIFY2(dock->sideTabWidget() == nullptr,
             "side-bar rail tab must be gone after un-pinning");
    QVERIFY2(!dock->isClosed(), "un-pinned dock must not be closed");
    QVERIFY2(dock->dockAreaWidget() != nullptr,
             "un-pinned dock must belong to an area again");
    QVERIFY2(dock->dockAreaWidget()->dockWidgets().contains(dock),
             "un-pinned dock must be listed by its area");
    QVERIFY2(dock->isVisible(), "un-pinned dock must be visible");
    QCOMPARE(m1->findDockWidget(QStringLiteral("tab1_NowPlaying")), dock);

    w.close();
    pump();
}

void TestDockInteractions::testAutoHideToggleBottom()
{
    DABAnalyserWindow w;
    w.show();
    pump();

    auto* mBottom = w.findChild<ads::CDockManager*>("bottomDockManager");
    QVERIFY(mBottom);

    ads::CDockWidget* dock = mBottom->findDockWidget(QStringLiteral("bottom_Logging"));
    QVERIFY(dock);
    QVERIFY(!dock->isAutoHide());

    dock->setAutoHide(true);
    pump();
    QVERIFY2(dock->isAutoHide(), "bottom Logging must report auto-hide after pinning");
    QVERIFY2(dock->autoHideDockContainer() != nullptr,
             "auto-hidden bottom dock must have a container");
    QVERIFY2(dock->sideTabWidget() != nullptr,
             "auto-hidden bottom dock must have a side-bar rail tab");
    QCOMPARE(mBottom->findDockWidget(QStringLiteral("bottom_Logging")), dock);

    dock->setAutoHide(false);
    QTRY_VERIFY_WITH_TIMEOUT(!dock->isAutoHide(), 5000);
    pump();
    QVERIFY2(!dock->isClosed(), "un-pinned bottom dock must not be closed");
    QVERIFY2(dock->dockAreaWidget() != nullptr,
             "un-pinned bottom dock must belong to an area again");
    // Restored as a current tab so it is genuinely visible (not merely a hidden
    // tab in a group).
    dock->setAsCurrentTab();
    pump();
    QVERIFY2(dock->isVisible(), "un-pinned bottom dock must be visible");
    QCOMPARE(mBottom->findDockWidget(QStringLiteral("bottom_Logging")), dock);

    w.close();
    pump();
}

// ============================================================================
// 3. MOVE ACROSS AREAS
// ============================================================================
void TestDockInteractions::testMoveAcrossAreasTab1()
{
    DABAnalyserWindow w;
    w.show();
    pump();

    auto* m1 = w.findChild<ads::CDockManager*>("tab1DockManager");
    QVERIFY(m1);
    ads::CDockWidget* dock = m1->findDockWidget(QStringLiteral("tab1_System"));
    QVERIFY(dock);

    QSplitter* root = rootOfDock(m1, dock);
    QVERIFY(root);
    ads::CDockAreaWidget* leftArea = dock->dockAreaWidget();
    QVERIFY(leftArea);
    const int leftColumn = columnIndexOf(root, leftArea);
    QCOMPARE(leftColumn, 0);  // left column

    // Move left -> right.
    m1->addDockWidget(ads::RightDockWidgetArea, dock, nullptr);
    pump();
    ads::CDockAreaWidget* rightArea = dock->dockAreaWidget();
    QVERIFY2(rightArea && rightArea != leftArea,
             "System must have moved to a new area");
    const int rightCol = columnIndexOf(root, rightArea);
    QVERIFY2(rightCol != leftColumn,
             qPrintable(QString("System must leave its original column %1 (now %2)")
                            .arg(leftColumn).arg(rightCol)));
    QVERIFY(dock->isVisible());
    QVERIFY(!dock->isClosed());

    // Move back right -> left.
    m1->addDockWidget(ads::LeftDockWidgetArea, dock, nullptr);
    pump();
    ads::CDockAreaWidget* backArea = dock->dockAreaWidget();
    QVERIFY(backArea);
    QVERIFY2(backArea != rightArea, "System must leave the moved area on the way back");
    const int backCol = columnIndexOf(root, backArea);
    QVERIFY2(backCol != rightCol,
             qPrintable(QString("System must leave the moved column %1 (now %2)")
                            .arg(rightCol).arg(backCol)));
    QVERIFY2(!dock->isClosed(), "System must survive the round trip");
    QCOMPARE(m1->findDockWidget(QStringLiteral("tab1_System")), dock);

    w.close();
    pump();
}

void TestDockInteractions::testMoveAcrossAreasBottom()
{
    DABAnalyserWindow w;
    w.show();
    pump();

    auto* mBottom = w.findChild<ads::CDockManager*>("bottomDockManager");
    QVERIFY(mBottom);
    ads::CDockWidget* dock = mBottom->findDockWidget(QStringLiteral("bottom_Constellation"));
    QVERIFY(dock);

    ads::CDockAreaWidget* originalArea = dock->dockAreaWidget();
    QVERIFY(originalArea);
    QVERIFY(originalArea->dockWidgetsCount() >= 7);

    // Move out of the shared bottom strip into a center area.
    mBottom->addDockWidget(ads::CenterDockWidgetArea, dock, nullptr);
    pump();
    ads::CDockAreaWidget* centerArea = dock->dockAreaWidget();
    QVERIFY2(centerArea && centerArea != originalArea,
             "Constellation must have moved to a new area");
    QCOMPARE(centerArea->dockWidgetsCount(), 1);
    QVERIFY(!dock->isClosed());

    // Move back into the bottom strip.
    mBottom->addDockWidget(ads::BottomDockWidgetArea, dock, nullptr);
    pump();
    QVERIFY2(dock->dockAreaWidget() != nullptr,
             "Constellation must be re-docked at the bottom");
    QVERIFY2(!dock->isClosed(), "Constellation must survive the round trip");
    QCOMPARE(mBottom->findDockWidget(QStringLiteral("bottom_Constellation")), dock);

    w.close();
    pump();
}

// ============================================================================
// 4. TABIFY / UNTABIFY
// ============================================================================
void TestDockInteractions::testTabifyAndUntabify()
{
    DABAnalyserWindow w;
    w.show();
    pump();

    auto* m1 = w.findChild<ads::CDockManager*>("tab1DockManager");
    QVERIFY(m1);

    // Two docks that start in SEPARATE areas.
    ads::CDockWidget* system = m1->findDockWidget(QStringLiteral("tab1_System"));
    ads::CDockWidget* nowPlaying = m1->findDockWidget(QStringLiteral("tab1_NowPlaying"));
    QVERIFY(system && nowPlaying);
    ads::CDockAreaWidget* systemArea = system->dockAreaWidget();
    ads::CDockAreaWidget* npArea = nowPlaying->dockAreaWidget();
    QVERIFY(systemArea && npArea);
    QVERIFY2(systemArea != npArea, "precondition: the two docks start in separate areas");
    QCOMPARE(systemArea->dockWidgetsCount(), 1);
    // Now Playing area now contains 4 docks: Now Playing / Slideshow, EPG, Journaline, TPEG
    QCOMPARE(npArea->dockWidgetsCount(), 4);

    // Tabify: merge Now Playing into the System area.
    // Only the Now Playing dock moves; EPG/Journaline/TPEG stay in the original area.
    m1->addDockWidgetTabToArea(nowPlaying, systemArea);
    pump();
    QCOMPARE(nowPlaying->dockAreaWidget(), systemArea);
    QCOMPARE(system->dockAreaWidget(), systemArea);
    QCOMPARE(systemArea->dockWidgetsCount(), 2);
    QVERIFY(systemArea->dockWidgets().contains(system));
    QVERIFY(systemArea->dockWidgets().contains(nowPlaying));
    // The original Now Playing area still has EPG, Journaline, TPEG (3 docks).
    QCOMPARE(npArea->dockWidgetsCount(), 3);

    // Untabify: move Now Playing back out into its own area.
    m1->addDockWidget(ads::CenterDockWidgetArea, nowPlaying, nullptr);
    pump();
    QVERIFY2(nowPlaying->dockAreaWidget() != systemArea,
             "moving a tab out must create a separate area again");
    QCOMPARE(system->dockAreaWidget()->dockWidgetsCount(), 1);
    // Now Playing is now in a new standalone area (1 dock).
    QCOMPARE(nowPlaying->dockAreaWidget()->dockWidgetsCount(), 1);
    QVERIFY(!nowPlaying->isClosed());
    QVERIFY(!system->isClosed());

    w.close();
    pump();
}

// ============================================================================
// 5. PERSISTENCE AFTER INTERACTIONS
// ============================================================================
void TestDockInteractions::testPersistenceAfterInteractions()
{
    // A non-default arrangement in the window-level BOTTOM manager (simple
    // single-root container, deterministic interactively):
    //   - bottom_Logging        floated
    //   - bottom_Constellation  moved out of the 7-tab strip into its own area
    //   - bottom_ErrorDetection tabified together with Constellation
    // After a close()/restart the floating + tab grouping + strip membership
    // must all come back.
    const QString loggingName = QStringLiteral("bottom_Logging");
    const QString movedName = QStringLiteral("bottom_Constellation");
    const QString errorDetName = QStringLiteral("bottom_ErrorDetection");
    const QString sysMsgName = QStringLiteral("bottom_SystemMessages");

    {
        DABAnalyserWindow w;
        w.show();
        pump();

        auto* mBottom = w.findChild<ads::CDockManager*>("bottomDockManager");
        QVERIFY(mBottom);

        ads::CDockWidget* logging = mBottom->findDockWidget(loggingName);
        ads::CDockWidget* moved = mBottom->findDockWidget(movedName);
        ads::CDockWidget* errorDet = mBottom->findDockWidget(errorDetName);
        QVERIFY(logging && moved && errorDet);
        ads::CDockAreaWidget* strip = moved->dockAreaWidget();
        QVERIFY(strip);
        QVERIFY(strip->dockWidgetsCount() >= 7);

        // Float one tab.
        logging->setFloating();
        // Move one tab out into its own center area.
        mBottom->addDockWidget(ads::CenterDockWidgetArea, moved, nullptr);
        // Tabify another tab into that new area.
        ads::CDockAreaWidget* newArea = moved->dockAreaWidget();
        QVERIFY(newArea && newArea != strip);
        mBottom->addDockWidgetTabToArea(errorDet, newArea);
        pump();

        QVERIFY2(logging->isInFloatingContainer(),
                 "bottom Logging must float before saving");
        QCOMPARE(moved->dockAreaWidget(), newArea);
        QCOMPARE(errorDet->dockAreaWidget(), newArea);
        QCOMPARE(newArea->dockWidgetsCount(), 2);
        QCOMPARE(strip->dockWidgetsCount(), 4);  // 7 - Logging - Constellation - ErrorDetection

        w.close();  // closeEvent() -> saveDockingState()
        pump();
    }

    {
        const QSettings s(QStringLiteral("StreamDAB-Analyser"), QStringLiteral("DABAnalyser"));
        QCOMPARE(s.value(QStringLiteral("docking/layout_version")).toInt(), 11);
        QVERIFY(!s.value(QStringLiteral("docking/bottom")).isNull());
    }

    {
        DABAnalyserWindow w2;
        w2.show();
        pump();

        auto* mBottom = w2.findChild<ads::CDockManager*>("bottomDockManager");
        QVERIFY(mBottom);

        ads::CDockWidget* logging = mBottom->findDockWidget(loggingName);
        ads::CDockWidget* moved = mBottom->findDockWidget(movedName);
        ads::CDockWidget* errorDet = mBottom->findDockWidget(errorDetName);
        ads::CDockWidget* sysMsg = mBottom->findDockWidget(sysMsgName);
        QVERIFY(logging && moved && errorDet && sysMsg);

        // Floating arrangement restored.
        QVERIFY2(logging->isInFloatingContainer(),
                 "floating arrangement must be restored after a restart");

        // Tab grouping restored: Constellation + Error Detection share an area.
        QVERIFY(moved->dockAreaWidget());
        QCOMPARE(errorDet->dockAreaWidget(), moved->dockAreaWidget());
        QCOMPARE(moved->dockAreaWidget()->dockWidgetsCount(), 2);

        // The moved pair is out of the shared strip; the strip keeps its
        // remaining 4 tabs (incl. System Messages).
        QVERIFY(sysMsg->dockAreaWidget());
        QVERIFY2(sysMsg->dockAreaWidget() != moved->dockAreaWidget(),
                 "the shared strip must remain separate from the moved area");
        QCOMPARE(sysMsg->dockAreaWidget()->dockWidgetsCount(), 4);

        w2.close();
        pump();
    }
}

// ============================================================================
// 6. REACHABILITY AFTER INTERACTIONS + RESTART
// ============================================================================
void TestDockInteractions::testAllDocksReachableAfterInteractionsAndRestart()
{
    const QStringList expected = expectedDockNames();
    QCOMPARE(expected.size(), 31);  // Phase 2A: +3 data-service docks; Phase 2C: +1 Advanced FIG Analyser dock

    // Window 1: shake the layout with one of each interaction.
    {
        DABAnalyserWindow w;
        w.show();
        pump();

        auto* m1 = w.findChild<ads::CDockManager*>("tab1DockManager");
        auto* mBottom = w.findChild<ads::CDockManager*>("bottomDockManager");
        QVERIFY(m1 && mBottom);

        ads::CDockWidget* floatDock = m1->findDockWidget(QStringLiteral("tab1_Status"));
        QVERIFY(floatDock);
        floatDock->setFloating();

        ads::CDockWidget* pinDock = m1->findDockWidget(QStringLiteral("tab1_Timing"));
        QVERIFY(pinDock);
        pinDock->setAutoHide(true);

        ads::CDockWidget* moveDock = m1->findDockWidget(QStringLiteral("tab1_System"));
        QVERIFY(moveDock);
        m1->addDockWidget(ads::RightDockWidgetArea, moveDock, nullptr);

        ads::CDockWidget* tabDock = mBottom->findDockWidget(QStringLiteral("bottom_Constellation"));
        ads::CDockWidget* host = mBottom->findDockWidget(QStringLiteral("bottom_Logging"));
        QVERIFY(tabDock && host);
        mBottom->addDockWidgetTabToArea(tabDock, host->dockAreaWidget());
        pump();

        // No dock was lost while interacting.
        QCOMPARE(totalDockCount(w), 31);  // Phase 2A: +3 data-service docks; Phase 2C: +1 Advanced FIG Analyser dock
        QSet<QString> live = dockNames(m1) + dockNames(mBottom)
                           + dockNames(w.findChild<ads::CDockManager*>("tab2DockManager"))
                           + dockNames(w.findChild<ads::CDockManager*>("tab3DockManager"));
        for (const QString& n : expected) {
            QVERIFY2(live.contains(n), qPrintable(QString("dock lost during interactions: %1").arg(n)));
        }

        w.close();  // save
        pump();
    }

    // Window 2: restart with the fully perturbed layout.
    {
        DABAnalyserWindow w2;
        w2.show();
        pump();

        const QList<ads::CDockManager*> managers = w2.findChildren<ads::CDockManager*>();
        QCOMPARE(managers.size(), 4);

        // 6a. Every expected dock is findable by objectName after the restart.
        for (const QString& name : expected) {
            ads::CDockWidget* found = nullptr;
            for (ads::CDockManager* m : managers) {
                if (ads::CDockWidget* d = m->findDockWidget(name)) {
                    found = d;
                    break;
                }
            }
            QVERIFY2(found, qPrintable(QString("dock not findable after restart: %1").arg(name)));
            QVERIFY2(!found->objectName().isEmpty(), "dock objectName must be preserved");
            QCOMPARE(found->objectName(), name);
        }

        // 6b. Every dock is toggleable (hide + show) without crash or loss.
        for (const QString& name : expected) {
            ads::CDockWidget* d = nullptr;
            for (ads::CDockManager* m : managers) {
                if (ads::CDockWidget* f = m->findDockWidget(name)) {
                    d = f;
                    break;
                }
            }
            QVERIFY(d);
            d->toggleView(false);
            pump();
            QVERIFY2(d->isClosed(), qPrintable(QString("toggleView(false) failed: %1").arg(name)));

            d->toggleView(true);
            pump();
            QVERIFY2(!d->isClosed(), qPrintable(QString("toggleView(true) failed: %1").arg(name)));
        }

        // Still 31 docks; none silently dropped.
        QCOMPARE(totalDockCount(w2), 31);  // Phase 2A: +3 data-service docks; Phase 2C: +1 Advanced FIG Analyser dock

        w2.close();
        pump();
    }
}

QTEST_MAIN(TestDockInteractions)
#include "test_dock_interactions.moc"
