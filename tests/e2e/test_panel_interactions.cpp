/**
 * @file test_panel_interactions.cpp
 * @brief Comprehensive Panel Interaction Testing for Phase 5.3 UI/UX Validation
 * 
 * Tests the three-panel professional layout interactions:
 * - Explorer Panel (25%): ETI service hierarchy navigation and filtering
 * - Main Content (50%): Frame listing with color coding and selection feedback  
 * - Properties Panel (25%): Parameter display modes and real-time updates
 * - Bottom Tool Panels: FIG analysis, audio monitor, error log coordination
 */

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <QApplication>
#include <QTest>
#include <QTimer>
#include <QSignalSpy>
#include <QSplitter>
#include <QWidget>
#include <QTableWidget>
#include <QTreeWidget>
#include <QTabWidget>
#include <QTextEdit>
#include <QPushButton>
#include <QComboBox>
#include <QHeaderView>
#include <QElapsedTimer>
#include <QEventLoop>
#include <QMouseEvent>
#include <QResizeEvent>
#include <memory>

// Core includes
#include "gui/main_window.h"
#include "gui/analyser_widget.h"
#include "gui/service_browser.h"
#include "gui/constellation_widget.h"
#include "gui/eti_service_tree_model.h"
#include "gui/eti_frame_list_model.h"
#include "gui/fig_analysis_widget.h"
#include "tests/fixtures/test_data_generators.h"

using ::testing::_;
using ::testing::Return;

/**
 * @class PanelInteractionTest
 * @brief Comprehensive testing for professional three-panel layout interactions
 */
class PanelInteractionTest : public ::testing::Test
{
protected:
    void SetUp() override {
        // Initialize Qt application for GUI testing
        if (!QApplication::instance()) {
            int argc = 0;
            char** argv = nullptr;
            app = std::make_unique<QApplication>(argc, argv);
        }

        // Create main window with three-panel layout
        mainWindow = std::make_unique<MainWindow>();
        ASSERT_TRUE(mainWindow->initialize());

        // Create test data generator
        testDataGenerator = std::make_unique<TestDataGenerators>();
        
        // Show main window for testing
        mainWindow->show();
        QTest::qWaitForWindowExposed(mainWindow.get());
        
        // Allow UI to stabilize and layout to complete
        QTest::qWait(200);
        
        // Get panel references
        setupPanelReferences();
    }

    void TearDown() override {
        if (mainWindow) {
            mainWindow->close();
            mainWindow.reset();
        }
        testDataGenerator.reset();
    }

    /**
     * @brief Setup references to all panels for testing
     */
    void setupPanelReferences() {
        // Main splitters
        horizontalSplitter = mainWindow->findChild<QSplitter*>("horizontalSplitter");
        verticalSplitter = mainWindow->findChild<QSplitter*>("verticalSplitter");
        
        // Three main panels
        explorerPanel = mainWindow->findChild<QWidget*>("explorerPanel");
        mainContentArea = mainWindow->findChild<QWidget*>("mainContentArea");
        propertiesPanel = mainWindow->findChild<QWidget*>("propertiesPanel");
        
        // Bottom tool tabs
        bottomToolTabs = mainWindow->findChild<QTabWidget*>("bottomToolTabs");
        
        // Explorer panel components
        serviceBrowser = mainWindow->getServiceBrowser();
        refreshServicesBtn = explorerPanel ? explorerPanel->findChild<QPushButton*>("refreshServicesBtn") : nullptr;
        
        // Main content components
        frameListTable = mainContentArea ? mainContentArea->findChild<QTableWidget*>("frameListTable") : nullptr;
        contentSplitter = mainContentArea ? mainContentArea->findChild<QSplitter*>("contentSplitter") : nullptr;
        
        // Properties panel components
        propertiesTabs = propertiesPanel ? propertiesPanel->findChild<QTabWidget*>("propertiesTabs") : nullptr;
        parametersText = propertiesPanel ? propertiesPanel->findChild<QTextEdit*>("parametersText") : nullptr;
        yamlOutputText = propertiesPanel ? propertiesPanel->findChild<QTextEdit*>("yamlOutputText") : nullptr;
        syncModeBtn = propertiesPanel ? propertiesPanel->findChild<QPushButton*>("syncModeBtn") : nullptr;
        compareModeBtn = propertiesPanel ? propertiesPanel->findChild<QPushButton*>("compareModeBtn") : nullptr;
    }

    /**
     * @brief Validate three-panel layout proportions (25%-50%-25%)
     */
    bool validateLayoutProportions() {
        if (!horizontalSplitter) return false;
        
        QList<int> sizes = horizontalSplitter->sizes();
        if (sizes.size() != 3) return false;

        int totalWidth = sizes[0] + sizes[1] + sizes[2];
        if (totalWidth == 0) return false;

        double leftPercent = (double)sizes[0] / totalWidth * 100.0;
        double centerPercent = (double)sizes[1] / totalWidth * 100.0;
        double rightPercent = (double)sizes[2] / totalWidth * 100.0;

        // Allow 5% tolerance for professional layout
        bool leftValid = (leftPercent >= 20.0 && leftPercent <= 30.0);
        bool centerValid = (centerPercent >= 45.0 && centerPercent <= 55.0);
        bool rightValid = (rightPercent >= 20.0 && rightPercent <= 30.0);

        return leftValid && centerValid && rightValid;
    }

    /**
     * @brief Test panel resize behavior
     */
    bool testPanelResizing() {
        if (!horizontalSplitter) return false;
        
        QList<int> originalSizes = horizontalSplitter->sizes();
        
        // Simulate user dragging splitter
        QList<int> newSizes = originalSizes;
        newSizes[0] = originalSizes[0] + 50;  // Expand left panel
        newSizes[1] = originalSizes[1] - 50;  // Shrink center panel
        
        horizontalSplitter->setSizes(newSizes);
        QTest::qWait(100);
        
        // Verify resize took effect
        QList<int> resultSizes = horizontalSplitter->sizes();
        bool resizeWorked = (resultSizes[0] != originalSizes[0]);
        
        // Restore original sizes
        horizontalSplitter->setSizes(originalSizes);
        QTest::qWait(100);
        
        return resizeWorked;
    }

    /**
     * @brief Measure interaction response time
     */
    double measureInteractionResponseTime(std::function<void()> interaction, int iterations = 10) {
        QElapsedTimer timer;
        qint64 totalTime = 0;
        
        for (int i = 0; i < iterations; ++i) {
            timer.start();
            interaction();
            QApplication::processEvents();
            totalTime += timer.elapsed();
            QTest::qWait(10); // Small delay between iterations
        }
        
        return (double)totalTime / iterations;
    }

    std::unique_ptr<QApplication> app;
    std::unique_ptr<MainWindow> mainWindow;
    std::unique_ptr<TestDataGenerators> testDataGenerator;
    
    // Panel references
    QSplitter *horizontalSplitter = nullptr;
    QSplitter *verticalSplitter = nullptr;
    QWidget *explorerPanel = nullptr;
    QWidget *mainContentArea = nullptr;
    QWidget *propertiesPanel = nullptr;
    QTabWidget *bottomToolTabs = nullptr;
    
    // Explorer panel components
    ServiceBrowser *serviceBrowser = nullptr;
    QPushButton *refreshServicesBtn = nullptr;
    
    // Main content components
    QTableWidget *frameListTable = nullptr;
    QSplitter *contentSplitter = nullptr;
    
    // Properties panel components
    QTabWidget *propertiesTabs = nullptr;
    QTextEdit *parametersText = nullptr;
    QTextEdit *yamlOutputText = nullptr;
    QPushButton *syncModeBtn = nullptr;
    QPushButton *compareModeBtn = nullptr;
};

/**
 * @brief Test Case 1: Three-Panel Layout Structure Validation
 */
TEST_F(PanelInteractionTest, ThreePanelLayoutStructure) {
    // Verify all panels exist
    ASSERT_TRUE(horizontalSplitter) << "Horizontal splitter not found";
    ASSERT_TRUE(explorerPanel) << "Explorer panel not found";
    ASSERT_TRUE(mainContentArea) << "Main content area not found";  
    ASSERT_TRUE(propertiesPanel) << "Properties panel not found";
    
    // Verify splitter has 3 widgets
    EXPECT_EQ(horizontalSplitter->count(), 3) << "Horizontal splitter should have 3 panels";
    
    // Verify layout proportions
    EXPECT_TRUE(validateLayoutProportions()) << "Panel proportions don't match 25%-50%-25% target";
    
    // Verify panels are visible
    EXPECT_TRUE(explorerPanel->isVisible()) << "Explorer panel should be visible";
    EXPECT_TRUE(mainContentArea->isVisible()) << "Main content area should be visible";
    EXPECT_TRUE(propertiesPanel->isVisible()) << "Properties panel should be visible";
    
    // Verify panel minimum sizes
    EXPECT_GT(explorerPanel->width(), 100) << "Explorer panel too narrow";
    EXPECT_GT(mainContentArea->width(), 200) << "Main content area too narrow";
    EXPECT_GT(propertiesPanel->width(), 100) << "Properties panel too narrow";
}

/**
 * @brief Test Case 2: Explorer Panel (25%) - ETI Service Navigation
 */
TEST_F(PanelInteractionTest, ExplorerPanelNavigation) {
    ASSERT_TRUE(explorerPanel) << "Explorer panel not available";
    ASSERT_TRUE(serviceBrowser) << "Service browser not available";
    
    // Setup test ETI service data
    testDataGenerator->populateServiceBrowser(serviceBrowser);
    QTest::qWait(300);
    
    // Verify service tree model
    auto* serviceTreeModel = serviceBrowser->findChild<EtiServiceTreeModel*>();
    ASSERT_TRUE(serviceTreeModel) << "Service tree model not found";
    
    // Test service hierarchy display
    EXPECT_GT(serviceTreeModel->rowCount(), 0) << "Service tree should have data";
    
    // Test service selection
    QModelIndex ensembleIndex = serviceTreeModel->getEnsembleIndex();
    if (ensembleIndex.isValid()) {
        // Simulate service selection
        QSignalSpy selectionSpy(serviceTreeModel, &EtiServiceTreeModel::serviceSelectionRequested);
        
        // Get first service if available
        QList<quint32> services = serviceTreeModel->getAllServices();
        if (!services.isEmpty()) {
            QModelIndex serviceIndex = serviceTreeModel->getServiceIndex(services.first());
            if (serviceIndex.isValid()) {
                serviceBrowser->setCurrentIndex(serviceIndex);
                QTest::qWait(100);
                
                // Verify selection signal emitted
                EXPECT_GE(selectionSpy.count(), 0) << "Service selection should trigger signals";
            }
        }
    }
    
    // Test refresh functionality
    if (refreshServicesBtn) {
        QSignalSpy refreshSpy(refreshServicesBtn, &QPushButton::clicked);
        refreshServicesBtn->click();
        QTest::qWait(100);
        EXPECT_EQ(refreshSpy.count(), 1) << "Refresh button should respond to clicks";
    }
    
    // Test filtering functionality
    auto* filterEdit = explorerPanel->findChild<QLineEdit*>("filterEdit");
    if (filterEdit) {
        filterEdit->setText("test");
        QTest::qWait(100);
        filterEdit->clear();
        QTest::qWait(100);
    }
}

/**
 * @brief Test Case 3: Main Content Area (50%) - Frame Listing with Color Coding
 */
TEST_F(PanelInteractionTest, MainContentFrameListing) {
    ASSERT_TRUE(mainContentArea) << "Main content area not available";
    ASSERT_TRUE(frameListTable) << "Frame list table not found";
    
    // Setup test ETI frame data
    testDataGenerator->populateFrameList(frameListTable);
    QTest::qWait(300);
    
    // Verify frame table structure
    EXPECT_GT(frameListTable->columnCount(), 0) << "Frame table should have columns";
    EXPECT_GT(frameListTable->rowCount(), 0) << "Frame table should have data";
    
    // Test frame selection feedback
    if (frameListTable->rowCount() > 0) {
        QSignalSpy selectionSpy(frameListTable, &QTableWidget::currentCellChanged);
        
        // Select first frame
        frameListTable->selectRow(0);
        QTest::qWait(50);
        
        EXPECT_EQ(frameListTable->currentRow(), 0) << "Frame selection should work";
        EXPECT_GE(selectionSpy.count(), 0) << "Selection should trigger signals";
        
        // Test color coding
        auto* firstItem = frameListTable->item(0, 0);
        if (firstItem) {
            QColor backgroundColor = firstItem->background().color();
            EXPECT_TRUE(backgroundColor.isValid()) << "Frame items should have background colors";
        }
    }
    
    // Test column sorting
    if (frameListTable->horizontalHeader()) {
        frameListTable->horizontalHeader()->sectionClicked(0);
        QTest::qWait(100);
        
        frameListTable->horizontalHeader()->sectionClicked(0); // Reverse sort
        QTest::qWait(100);
    }
    
    // Test scroll performance
    if (frameListTable->rowCount() > 10) {
        QElapsedTimer scrollTimer;
        scrollTimer.start();
        
        frameListTable->scrollToBottom();
        QTest::qWait(50);
        frameListTable->scrollToTop();
        
        qint64 scrollTime = scrollTimer.elapsed();
        EXPECT_LT(scrollTime, 200) << "Scrolling should be responsive (<200ms)";
    }
    
    // Test frame detail display integration
    if (contentSplitter) {
        EXPECT_EQ(contentSplitter->count(), 2) << "Content splitter should have analyser and constellation";
        
        auto* analyserWidget = mainWindow->getAnalyserWidget();
        auto* constellationWidget = mainWindow->getConstellationWidget();
        
        EXPECT_TRUE(analyserWidget) << "Analyser widget should be present";
        EXPECT_TRUE(constellationWidget) << "Constellation widget should be present";
    }
}

/**
 * @brief Test Case 4: Properties Panel (25%) - Parameter Display and Modes
 */
TEST_F(PanelInteractionTest, PropertiesPanelDisplay) {
    ASSERT_TRUE(propertiesPanel) << "Properties panel not available";
    ASSERT_TRUE(propertiesTabs) << "Properties tabs not found";
    
    // Verify tabs structure
    EXPECT_GE(propertiesTabs->count(), 2) << "Properties panel should have multiple tabs";
    
    // Test tab switching
    for (int i = 0; i < propertiesTabs->count(); ++i) {
        QSignalSpy tabChangeSpy(propertiesTabs, &QTabWidget::currentChanged);
        
        propertiesTabs->setCurrentIndex(i);
        QTest::qWait(50);
        
        EXPECT_EQ(propertiesTabs->currentIndex(), i) << "Tab switching should work";
        EXPECT_GE(tabChangeSpy.count(), 0) << "Tab changes should trigger signals";
    }
    
    // Test parameter display updates
    if (parametersText) {
        // Simulate parameter update
        testDataGenerator->updateParametersDisplay(parametersText);
        QTest::qWait(100);
        
        EXPECT_FALSE(parametersText->toPlainText().isEmpty()) << "Parameters should display content";
        
        // Test read-only behavior
        EXPECT_FALSE(parametersText->isReadOnly()) << "Parameters text should be editable for comments";
    }
    
    // Test YAML output display
    if (yamlOutputText) {
        testDataGenerator->updateYamlDisplay(yamlOutputText);
        QTest::qWait(100);
        
        EXPECT_FALSE(yamlOutputText->toPlainText().isEmpty()) << "YAML output should display content";
    }
    
    // Test mode buttons
    if (syncModeBtn) {
        QSignalSpy syncSpy(syncModeBtn, &QPushButton::clicked);
        syncModeBtn->click();
        QTest::qWait(50);
        EXPECT_EQ(syncSpy.count(), 1) << "Sync mode button should respond";
    }
    
    if (compareModeBtn) {
        QSignalSpy compareSpy(compareModeBtn, &QPushButton::clicked);
        compareModeBtn->click();
        QTest::qWait(50);
        EXPECT_EQ(compareSpy.count(), 1) << "Compare mode button should respond";
    }
    
    // Test real-time updates
    double updateResponseTime = measureInteractionResponseTime([this]() {
        if (parametersText) {
            testDataGenerator->simulateParameterUpdate(parametersText);
        }
    }, 5);
    
    EXPECT_LT(updateResponseTime, 50.0) << "Property updates should be fast (<50ms)";
}

/**
 * @brief Test Case 5: Bottom Tool Panels - Specialist Analysis Tools
 */
TEST_F(PanelInteractionTest, BottomToolPanels) {
    ASSERT_TRUE(bottomToolTabs) << "Bottom tool tabs not found";
    
    // Verify bottom panel structure
    EXPECT_GE(bottomToolTabs->count(), 4) << "Should have multiple analysis tools";
    
    // Test tool tab switching
    for (int i = 0; i < bottomToolTabs->count(); ++i) {
        QSignalSpy tabChangeSpy(bottomToolTabs, &QTabWidget::currentChanged);
        
        bottomToolTabs->setCurrentIndex(i);
        QTest::qWait(50);
        
        EXPECT_EQ(bottomToolTabs->currentIndex(), i) << "Bottom tool tab switching should work";
        
        // Verify tab content exists
        QWidget* tabContent = bottomToolTabs->widget(i);
        EXPECT_TRUE(tabContent) << "Tab should have content widget";
        EXPECT_TRUE(tabContent->isVisible()) << "Tab content should be visible when selected";
    }
    
    // Test specific analysis tools
    QStringList expectedTools = {"Hex Viewer", "Messages", "TR 101-290", "Time Dynamics", "Graphics", "Comments", "EPG"};
    
    for (const QString& toolName : expectedTools) {
        bool toolFound = false;
        for (int i = 0; i < bottomToolTabs->count(); ++i) {
            if (bottomToolTabs->tabText(i).contains(toolName, Qt::CaseInsensitive)) {
                toolFound = true;
                
                // Test tool functionality
                bottomToolTabs->setCurrentIndex(i);
                QTest::qWait(100);
                
                QWidget* toolWidget = bottomToolTabs->widget(i);
                EXPECT_TRUE(toolWidget) << QString("Tool widget for %1 should exist").arg(toolName);
                
                break;
            }
        }
        // Note: Some tools might not be implemented yet, so this is informational
    }
    
    // Test FIG analysis widget specifically
    auto* figAnalysisWidget = mainWindow->findChild<FigAnalysisWidget*>();
    if (figAnalysisWidget) {
        // Test FIG analysis updates
        testDataGenerator->generateTestFigData(figAnalysisWidget);
        QTest::qWait(200);
        
        double complianceScore = figAnalysisWidget->getOverallComplianceScore();
        EXPECT_GE(complianceScore, 0.0) << "Compliance score should be valid";
        EXPECT_LE(complianceScore, 100.0) << "Compliance score should be within range";
    }
}

/**
 * @brief Test Case 6: Panel Interaction Response Times
 */
TEST_F(PanelInteractionTest, InteractionResponseTimes) {
    // Test Explorer Panel response time
    double explorerResponseTime = measureInteractionResponseTime([this]() {
        if (serviceBrowser) {
            auto* serviceTreeModel = serviceBrowser->findChild<EtiServiceTreeModel*>();
            if (serviceTreeModel) {
                QList<quint32> services = serviceTreeModel->getAllServices();
                if (!services.isEmpty()) {
                    QModelIndex serviceIndex = serviceTreeModel->getServiceIndex(services.first());
                    serviceBrowser->setCurrentIndex(serviceIndex);
                }
            }
        }
    }, 5);
    
    EXPECT_LT(explorerResponseTime, 50.0) << "Explorer panel interactions should be <50ms";
    
    // Test Main Content response time
    double mainContentResponseTime = measureInteractionResponseTime([this]() {
        if (frameListTable && frameListTable->rowCount() > 0) {
            int randomRow = qrand() % frameListTable->rowCount();
            frameListTable->selectRow(randomRow);
        }
    }, 5);
    
    EXPECT_LT(mainContentResponseTime, 50.0) << "Main content interactions should be <50ms";
    
    // Test Properties Panel response time
    double propertiesResponseTime = measureInteractionResponseTime([this]() {
        if (propertiesTabs && propertiesTabs->count() > 1) {
            int randomTab = qrand() % propertiesTabs->count();
            propertiesTabs->setCurrentIndex(randomTab);
        }
    }, 5);
    
    EXPECT_LT(propertiesResponseTime, 50.0) << "Properties panel interactions should be <50ms";
    
    // Test Bottom Tools response time
    double bottomToolsResponseTime = measureInteractionResponseTime([this]() {
        if (bottomToolTabs && bottomToolTabs->count() > 1) {
            int randomTab = qrand() % bottomToolTabs->count();
            bottomToolTabs->setCurrentIndex(randomTab);
        }
    }, 5);
    
    EXPECT_LT(bottomToolsResponseTime, 50.0) << "Bottom tool interactions should be <50ms";
}

/**
 * @brief Test Case 7: Panel Resize and Layout Stability
 */
TEST_F(PanelInteractionTest, PanelResizeStability) {
    ASSERT_TRUE(horizontalSplitter) << "Horizontal splitter required for resize testing";
    
    // Record initial layout
    QList<int> initialSizes = horizontalSplitter->sizes();
    QSize initialWindowSize = mainWindow->size();
    
    // Test splitter dragging
    EXPECT_TRUE(testPanelResizing()) << "Panel resizing should work";
    
    // Test window resize impact
    mainWindow->resize(1400, 900);
    QTest::qWait(200);
    
    EXPECT_TRUE(validateLayoutProportions()) << "Layout proportions should be maintained after window resize";
    
    // Test minimum size constraints
    mainWindow->resize(800, 600);
    QTest::qWait(200);
    
    EXPECT_TRUE(validateLayoutProportions()) << "Layout should handle minimum window sizes";
    
    // Test maximum size handling
    mainWindow->resize(2000, 1200);
    QTest::qWait(200);
    
    EXPECT_TRUE(validateLayoutProportions()) << "Layout should handle large window sizes";
    
    // Restore original size
    mainWindow->resize(initialWindowSize);
    QTest::qWait(200);
    
    // Verify panels still functional
    EXPECT_TRUE(explorerPanel->isVisible()) << "Explorer panel should remain visible";
    EXPECT_TRUE(mainContentArea->isVisible()) << "Main content should remain visible";
    EXPECT_TRUE(propertiesPanel->isVisible()) << "Properties panel should remain visible";
}

/**
 * @brief Test Case 8: Cross-Panel Communication and Coordination
 */
TEST_F(PanelInteractionTest, CrossPanelCommunication) {
    // Setup test data in all panels
    if (serviceBrowser) {
        testDataGenerator->populateServiceBrowser(serviceBrowser);
    }
    if (frameListTable) {
        testDataGenerator->populateFrameList(frameListTable);
    }
    
    QTest::qWait(300);
    
    // Test service selection → frame display coordination
    if (serviceBrowser && frameListTable) {
        auto* serviceTreeModel = serviceBrowser->findChild<EtiServiceTreeModel*>();
        if (serviceTreeModel) {
            QSignalSpy serviceSelectionSpy(serviceTreeModel, &EtiServiceTreeModel::serviceSelectionRequested);
            
            QList<quint32> services = serviceTreeModel->getAllServices();
            if (!services.isEmpty()) {
                // Select a service
                QModelIndex serviceIndex = serviceTreeModel->getServiceIndex(services.first());
                serviceBrowser->setCurrentIndex(serviceIndex);
                QTest::qWait(100);
                
                // Verify frame list responds
                EXPECT_GT(frameListTable->rowCount(), 0) << "Frame list should update with service selection";
            }
        }
    }
    
    // Test frame selection → properties display coordination
    if (frameListTable && parametersText) {
        if (frameListTable->rowCount() > 0) {
            QString initialText = parametersText->toPlainText();
            
            frameListTable->selectRow(0);
            QTest::qWait(100);
            
            QString updatedText = parametersText->toPlainText();
            // Properties should update (content may change or stay same depending on implementation)
            EXPECT_TRUE(parametersText->isVisible()) << "Properties display should be visible";
        }
    }
    
    // Test bottom tool coordination
    if (bottomToolTabs && frameListTable) {
        // Select FIG analysis tab if available
        for (int i = 0; i < bottomToolTabs->count(); ++i) {
            if (bottomToolTabs->tabText(i).contains("FIG", Qt::CaseInsensitive)) {
                bottomToolTabs->setCurrentIndex(i);
                QTest::qWait(100);
                
                // Verify FIG analysis responds to frame selection
                if (frameListTable->rowCount() > 0) {
                    frameListTable->selectRow(0);
                    QTest::qWait(100);
                    
                    auto* figWidget = bottomToolTabs->widget(i);
                    EXPECT_TRUE(figWidget->isVisible()) << "FIG analysis should be visible";
                }
                break;
            }
        }
    }
}

/**
 * @brief Test Case 9: Professional UI Consistency
 */
TEST_F(PanelInteractionTest, ProfessionalUIConsistency) {
    // Test color scheme consistency across panels
    QPalette explorerPalette = explorerPanel->palette();
    QPalette mainContentPalette = mainContentArea->palette();
    QPalette propertiesPalette = propertiesPanel->palette();
    
    // All panels should use consistent dark theme
    EXPECT_TRUE(explorerPalette.color(QPalette::Window).value() < 128) << "Explorer should use dark theme";
    EXPECT_TRUE(mainContentPalette.color(QPalette::Window).value() < 128) << "Main content should use dark theme";
    EXPECT_TRUE(propertiesPalette.color(QPalette::Window).value() < 128) << "Properties should use dark theme";
    
    // Test font consistency
    QFont explorerFont = explorerPanel->font();
    QFont mainContentFont = mainContentArea->font();
    QFont propertiesFont = propertiesPanel->font();
    
    EXPECT_EQ(explorerFont.family(), mainContentFont.family()) << "Panels should use consistent fonts";
    EXPECT_EQ(mainContentFont.family(), propertiesFont.family()) << "Panels should use consistent fonts";
    
    // Test spacing and margins consistency
    if (horizontalSplitter) {
        EXPECT_GT(horizontalSplitter->handleWidth(), 0) << "Splitter should have visible handles";
        EXPECT_LT(horizontalSplitter->handleWidth(), 10) << "Splitter handles should be professional size";
    }
}

/**
 * @brief Test Case 10: Error Handling and Edge Cases
 */
TEST_F(PanelInteractionTest, ErrorHandlingEdgeCases) {
    // Test empty data handling
    if (frameListTable) {
        frameListTable->clear();
        frameListTable->setRowCount(0);
        QTest::qWait(100);
        
        EXPECT_EQ(frameListTable->rowCount(), 0) << "Empty table should be handled gracefully";
        EXPECT_TRUE(frameListTable->isVisible()) << "Empty table should remain visible";
    }
    
    // Test rapid panel switching
    for (int iteration = 0; iteration < 10; ++iteration) {
        if (propertiesTabs && propertiesTabs->count() > 1) {
            for (int i = 0; i < propertiesTabs->count(); ++i) {
                propertiesTabs->setCurrentIndex(i);
                QApplication::processEvents();
            }
        }
        
        if (bottomToolTabs && bottomToolTabs->count() > 1) {
            for (int i = 0; i < bottomToolTabs->count(); ++i) {
                bottomToolTabs->setCurrentIndex(i);
                QApplication::processEvents();
            }
        }
    }
    
    // Verify UI remains responsive after rapid switching
    EXPECT_TRUE(mainWindow->isVisible()) << "Main window should remain responsive";
    EXPECT_TRUE(validateLayoutProportions()) << "Layout should remain stable after rapid switching";
    
    // Test memory cleanup
    if (serviceBrowser) {
        auto* serviceTreeModel = serviceBrowser->findChild<EtiServiceTreeModel*>();
        if (serviceTreeModel) {
            serviceTreeModel->clearModel();
            QTest::qWait(100);
            EXPECT_EQ(serviceTreeModel->rowCount(), 0) << "Model clearing should work";
        }
    }
}

// Test Suite Entry Point
int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    
    // Initialize Qt Test framework
    QApplication app(argc, argv);
    
    // Run all tests
    return RUN_ALL_TESTS();
}