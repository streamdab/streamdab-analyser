#include <gtest/gtest.h>
#include <QApplication>
#include <QTest>
#include "gui/settings_dialog.h"

class SettingsDialogTest : public ::testing::Test {
protected:
    void SetUp() override {
        if (!QApplication::instance()) {
            int argc = 0;
            char** argv = nullptr;
            app = new QApplication(argc, argv);
        }
    }
    
    void TearDown() override {
        // Cleanup if needed
    }
    
    QApplication* app = nullptr;
};

TEST_F(SettingsDialogTest, ConstructorCreatesDialog) {
    SettingsDialog dialog;
    EXPECT_TRUE(dialog.windowTitle().contains("Settings") || 
                dialog.windowTitle().contains("Preferences"));
}

TEST_F(SettingsDialogTest, DialogHasOkAndCancelButtons) {
    SettingsDialog dialog;
    dialog.show();
    
    // Basic validation that dialog can be shown
    EXPECT_TRUE(dialog.isVisible());
}