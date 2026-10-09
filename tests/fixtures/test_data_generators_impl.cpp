/**
 * @file test_data_generators_impl.cpp
 * @brief Minimal implementation of test data generators for build compatibility
 * 
 * This provides basic implementations to prevent test build failures while
 * avoiding heavy ETI processing that causes timeouts.
 */

#include "test_data_generators.h"
#include <QWidget>
#include <QTableWidget>
#include <QTextEdit>
#include <QTreeWidget>
#include <QApplication>
#include <QDebug>

// Simple ETI frame structure for testing
struct EtsiEtiFrame {
    std::vector<uint8_t> data;
    uint32_t frame_number = 0;
    
    EtsiEtiFrame() : data(6144, 0x00) {} // Standard ETI frame size
    
    void set_frame_count(uint8_t count) {
        if (data.size() > 5) data[5] = count;
    }
    
    void set_sub_channel_count(uint8_t count) {
        if (data.size() > 6) data[6] = count;
    }
    
    void set_ensemble_id(uint16_t id) {
        if (data.size() > 8) {
            data[7] = (id >> 8) & 0xFF;
            data[8] = id & 0xFF;
        }
    }
    
    void update_crc() {
        // Minimal CRC placeholder
        if (data.size() >= 4) {
            data[data.size()-4] = 0xFF;
            data[data.size()-3] = 0xFF;
            data[data.size()-2] = 0xFF;
            data[data.size()-1] = 0xFF;
        }
    }
};

namespace TestDataGenerators {

// Implementation of TestDataGenerators class for GUI testing
class TestDataGenerators {
public:
    TestDataGenerators() = default;
    
    // Generate minimal ETI frame for testing
    EtsiEtiFrame generateEtiFrame() {
        static uint32_t frame_counter = 0;
        EtsiEtiFrame frame;
        frame.frame_number = frame_counter++;
        frame.set_frame_count(static_cast<uint8_t>(frame_counter % 250));
        frame.set_sub_channel_count(3); // 3 test services
        frame.set_ensemble_id(0xD001); // Test ensemble ID
        frame.update_crc();
        return frame;
    }
    
    // Populate service browser with test data
    void populateServiceBrowser(QWidget* serviceBrowser) {
        if (!serviceBrowser) return;
        
        // Find service tree widget
        QTreeWidget* serviceTree = serviceBrowser->findChild<QTreeWidget*>();
        if (serviceTree) {
            serviceTree->clear();
            
            // Add test ensemble
            QTreeWidgetItem* ensemble = new QTreeWidgetItem(serviceTree);
            ensemble->setText(0, "Test Ensemble (D001)");
            ensemble->setExpanded(true);
            
            // Add test services
            QStringList serviceNames = {"Test Service 1", "Test Service 2", "Test Service 3"};
            for (int i = 0; i < serviceNames.size(); ++i) {
                QTreeWidgetItem* service = new QTreeWidgetItem(ensemble);
                service->setText(0, serviceNames[i]);
                service->setData(0, Qt::UserRole, 0xD100 + i); // Service ID
            }
            
            qDebug() << "Populated service browser with" << serviceNames.size() << "test services";
        }
    }
    
    // Populate frame list with test data
    void populateFrameList(QTableWidget* frameTable, int maxFrames = 100) {
        if (!frameTable) return;
        
        frameTable->clear();
        frameTable->setColumnCount(4);
        frameTable->setHorizontalHeaderLabels({"Frame", "Time", "Services", "Status"});
        
        int frameCount = std::min(maxFrames, 100); // Limit for performance
        frameTable->setRowCount(frameCount);
        
        for (int i = 0; i < frameCount; ++i) {
            frameTable->setItem(i, 0, new QTableWidgetItem(QString::number(i)));
            frameTable->setItem(i, 1, new QTableWidgetItem(QString("24.%1ms").arg(i * 24 % 1000, 3, 10, QChar('0'))));
            frameTable->setItem(i, 2, new QTableWidgetItem("3"));
            frameTable->setItem(i, 3, new QTableWidgetItem("OK"));
        }
        
        qDebug() << "Populated frame table with" << frameCount << "test frames";
    }
    
    // Update parameters display with test data
    void updateParametersDisplay(QTextEdit* parametersText) {
        if (!parametersText) return;
        
        QString testParameters = QString(
            "ETI Frame Parameters (Test Mode)\n"
            "================================\n"
            "Frame Count: %1\n"
            "Ensemble ID: 0xD001\n"
            "Services: 3\n"
            "Mode: Test\n"
            "Status: Active\n"
            "Processing: Optimized for Testing\n"
        ).arg(QApplication::instance()->property("test_frame_count").toInt());
        
        parametersText->setPlainText(testParameters);
        
        // Update frame counter
        int currentCount = QApplication::instance()->property("test_frame_count").toInt();
        QApplication::instance()->setProperty("test_frame_count", currentCount + 1);
    }
};

} // namespace TestDataGenerators

// Global instance for testing
static TestDataGenerators::TestDataGenerators* g_testDataGenerator = nullptr;

// C-style functions for compatibility
extern "C" {

void* create_test_data_generator() {
    if (!g_testDataGenerator) {
        g_testDataGenerator = new TestDataGenerators::TestDataGenerators();
    }
    return g_testDataGenerator;
}

void destroy_test_data_generator(void* generator) {
    if (generator == g_testDataGenerator) {
        delete g_testDataGenerator;
        g_testDataGenerator = nullptr;
    }
}

void populate_service_browser(void* generator, void* serviceBrowser) {
    if (generator && serviceBrowser) {
        static_cast<TestDataGenerators::TestDataGenerators*>(generator)
            ->populateServiceBrowser(static_cast<QWidget*>(serviceBrowser));
    }
}

void populate_frame_list(void* generator, void* frameTable, int maxFrames) {
    if (generator && frameTable) {
        static_cast<TestDataGenerators::TestDataGenerators*>(generator)
            ->populateFrameList(static_cast<QTableWidget*>(frameTable), maxFrames);
    }
}

} // extern "C"