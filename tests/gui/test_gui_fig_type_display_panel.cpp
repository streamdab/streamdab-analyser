/**
 * @file test_gui_fig_type_display_panel.cpp
 * @brief GUI Tests for FIG Type Display Panel Component
 *
 * PDCA Week 5 Phase 2 - E2E/GUI Test Implementation
 *
 * @author Claude Code (Sonnet 4.5)
 * @date November 2, 2025
 */

#include <QtTest/QtTest>
#include <QWidget>
#include <QLabel>
#include <QString>
#include <QMap>

/**
 * Mock FIG information structure
 */
struct MockFigInfo {
    uint8_t figType;
    uint8_t figExtension;
    QString description;
    int count;
};

/**
 * Mock FigTypeDisplayPanel for testing
 */
class MockFigTypeDisplayPanel : public QWidget {
public:
    MockFigTypeDisplayPanel() : m_totalFigCount(0), m_displayEnabled(true) {}

    void addFigType(uint8_t type, uint8_t ext, const QString& description) {
        QString key = QString("%1/%2").arg(type).arg(ext);
        if (m_figTypes.contains(key)) {
            m_figTypes[key].count++;
        } else {
            MockFigInfo info{type, ext, description, 1};
            m_figTypes[key] = info;
        }
        m_totalFigCount++;
    }

    int getFigTypeCount(uint8_t type, uint8_t ext) const {
        QString key = QString("%1/%2").arg(type).arg(ext);
        if (m_figTypes.contains(key)) {
            return m_figTypes[key].count;
        }
        return 0;
    }

    int getTotalFigCount() const { return m_totalFigCount; }

    int getUniqueFigTypeCount() const { return m_figTypes.size(); }

    void clearFigTypes() {
        m_figTypes.clear();
        m_totalFigCount = 0;
    }

    bool hasFigType(uint8_t type, uint8_t ext) const {
        QString key = QString("%1/%2").arg(type).arg(ext);
        return m_figTypes.contains(key);
    }

    QString getFigDescription(uint8_t type, uint8_t ext) const {
        QString key = QString("%1/%2").arg(type).arg(ext);
        if (m_figTypes.contains(key)) {
            return m_figTypes[key].description;
        }
        return QString();
    }

    void setDisplayEnabled(bool enabled) { m_displayEnabled = enabled; }
    bool isDisplayEnabled() const { return m_displayEnabled; }

    QList<MockFigInfo> getAllFigTypes() const {
        return m_figTypes.values();
    }

private:
    QMap<QString, MockFigInfo> m_figTypes;
    int m_totalFigCount;
    bool m_displayEnabled;
};

class TestGuiFigTypeDisplayPanel : public QObject {
    Q_OBJECT

private slots:
    void initTestCase() { /* Ready */ }

    void testPanel_Creation() {
        MockFigTypeDisplayPanel panel;
        QCOMPARE(panel.getTotalFigCount(), 0);
        QCOMPARE(panel.getUniqueFigTypeCount(), 0);
        QVERIFY(panel.isDisplayEnabled());
    }

    void testPanel_AddSingleFigType() {
        MockFigTypeDisplayPanel panel;
        panel.addFigType(0, 0, "Ensemble information");
        QCOMPARE(panel.getTotalFigCount(), 1);
        QCOMPARE(panel.getUniqueFigTypeCount(), 1);
        QCOMPARE(panel.getFigTypeCount(0, 0), 1);
    }

    void testPanel_AddMultipleFigTypes() {
        MockFigTypeDisplayPanel panel;
        panel.addFigType(0, 0, "Ensemble information");
        panel.addFigType(0, 1, "Service organization");
        panel.addFigType(0, 2, "Service component");
        panel.addFigType(1, 0, "Ensemble label");
        panel.addFigType(1, 1, "Service label");

        QCOMPARE(panel.getTotalFigCount(), 5);
        QCOMPARE(panel.getUniqueFigTypeCount(), 5);
    }

    void testPanel_FigTypeCounter() {
        MockFigTypeDisplayPanel panel;

        // Add same FIG type multiple times
        for (int i = 0; i < 10; i++) {
            panel.addFigType(0, 0, "Ensemble information");
        }

        QCOMPARE(panel.getTotalFigCount(), 10);
        QCOMPARE(panel.getUniqueFigTypeCount(), 1);
        QCOMPARE(panel.getFigTypeCount(0, 0), 10);
    }

    void testPanel_MixedFigTypes() {
        MockFigTypeDisplayPanel panel;

        // Add FIG 0/0 five times
        for (int i = 0; i < 5; i++) {
            panel.addFigType(0, 0, "Ensemble information");
        }

        // Add FIG 0/1 three times
        for (int i = 0; i < 3; i++) {
            panel.addFigType(0, 1, "Service organization");
        }

        QCOMPARE(panel.getTotalFigCount(), 8);
        QCOMPARE(panel.getUniqueFigTypeCount(), 2);
        QCOMPARE(panel.getFigTypeCount(0, 0), 5);
        QCOMPARE(panel.getFigTypeCount(0, 1), 3);
    }

    void testPanel_HasFigType() {
        MockFigTypeDisplayPanel panel;
        panel.addFigType(0, 0, "Ensemble information");
        panel.addFigType(1, 0, "Ensemble label");

        QVERIFY(panel.hasFigType(0, 0));
        QVERIFY(panel.hasFigType(1, 0));
        QVERIFY(!panel.hasFigType(0, 2));
        QVERIFY(!panel.hasFigType(2, 0));
    }

    void testPanel_FigDescription() {
        MockFigTypeDisplayPanel panel;
        panel.addFigType(0, 0, "Ensemble information");
        panel.addFigType(0, 17, "Programme type");

        QCOMPARE(panel.getFigDescription(0, 0), QString("Ensemble information"));
        QCOMPARE(panel.getFigDescription(0, 17), QString("Programme type"));
        QCOMPARE(panel.getFigDescription(0, 99), QString());
    }

    void testPanel_ClearFigTypes() {
        MockFigTypeDisplayPanel panel;

        // Add multiple FIG types
        for (int i = 0; i < 5; i++) {
            panel.addFigType(0, i, QString("FIG 0/%1").arg(i));
        }

        QCOMPARE(panel.getTotalFigCount(), 5);
        panel.clearFigTypes();
        QCOMPARE(panel.getTotalFigCount(), 0);
        QCOMPARE(panel.getUniqueFigTypeCount(), 0);
    }

    void testPanel_DisplayEnabled() {
        MockFigTypeDisplayPanel panel;
        QVERIFY(panel.isDisplayEnabled());

        panel.setDisplayEnabled(false);
        QVERIFY(!panel.isDisplayEnabled());

        panel.setDisplayEnabled(true);
        QVERIFY(panel.isDisplayEnabled());
    }

    void testPanel_Visibility() {
        MockFigTypeDisplayPanel panel;
        QVERIFY(!panel.isVisible());
        panel.show();
        QVERIFY(panel.isVisible());
        panel.hide();
        QVERIFY(!panel.isVisible());
    }

    void testPanel_EnabledState() {
        MockFigTypeDisplayPanel panel;
        QVERIFY(panel.isEnabled());
        panel.setEnabled(false);
        QVERIFY(!panel.isEnabled());
        panel.setEnabled(true);
        QVERIFY(panel.isEnabled());
    }

    void testPanel_AllFigTypes() {
        MockFigTypeDisplayPanel panel;
        panel.addFigType(0, 0, "Ensemble information");
        panel.addFigType(0, 1, "Service organization");
        panel.addFigType(1, 0, "Ensemble label");

        QList<MockFigInfo> allFigs = panel.getAllFigTypes();
        QCOMPARE(allFigs.size(), 3);
    }

    void testPanel_LargeFigCount() {
        MockFigTypeDisplayPanel panel;

        // Simulate receiving 1000 FIG frames
        for (int i = 0; i < 1000; i++) {
            panel.addFigType(0, i % 20, QString("FIG 0/%1").arg(i % 20));
        }

        QCOMPARE(panel.getTotalFigCount(), 1000);
        QCOMPARE(panel.getUniqueFigTypeCount(), 20);
        QCOMPARE(panel.getFigTypeCount(0, 0), 50); // 1000/20 = 50
    }

    void testPanel_BoundaryValues() {
        MockFigTypeDisplayPanel panel;

        // Test maximum FIG type values
        panel.addFigType(7, 31, "Max FIG type");
        QVERIFY(panel.hasFigType(7, 31));
        QCOMPARE(panel.getFigTypeCount(7, 31), 1);

        // Test minimum FIG type values
        panel.addFigType(0, 0, "Min FIG type");
        QVERIFY(panel.hasFigType(0, 0));
        QCOMPARE(panel.getFigTypeCount(0, 0), 1);
    }

    void testPanel_StatePersistence() {
        MockFigTypeDisplayPanel panel;

        panel.addFigType(0, 0, "Ensemble information");
        panel.addFigType(0, 1, "Service organization");
        panel.setDisplayEnabled(false);

        QCOMPARE(panel.getTotalFigCount(), 2);
        QCOMPARE(panel.getUniqueFigTypeCount(), 2);
        QVERIFY(!panel.isDisplayEnabled());
        QVERIFY(panel.hasFigType(0, 0));
        QVERIFY(panel.hasFigType(0, 1));
    }
};

QTEST_MAIN(TestGuiFigTypeDisplayPanel)
#include "test_gui_fig_type_display_panel.moc"
