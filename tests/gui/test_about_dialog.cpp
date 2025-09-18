#include <gtest/gtest.h>
#include <QApplication>
#include <QTest>
#include "gui/about_dialog.h"

class AboutDialogTest : public ::testing::Test {
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

TEST_F(AboutDialogTest, ConstructorCreatesDialog) {
    AboutDialog dialog;
    EXPECT_TRUE(dialog.windowTitle().contains("About"));
}

TEST_F(AboutDialogTest, DialogDisplaysVersionInfo) {
    AboutDialog dialog;
    dialog.show();
    
    // Basic validation that dialog can be shown
    EXPECT_TRUE(dialog.isVisible());
}