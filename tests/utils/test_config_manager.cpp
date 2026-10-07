#include <gtest/gtest.h>
#include <QSignalSpy>
#include <QTest>
#include <QTemporaryDir>
#include <QStandardPaths>
#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include "../../src/utils/config_manager.h"

class ConfigManagerTest : public ::testing::Test 
{
protected:
    void SetUp() override 
    {
        // Create temporary directory for test settings
        tempDir = new QTemporaryDir();
        ASSERT_TRUE(tempDir->isValid());
        
        // Override settings location to temporary directory
        originalAppName = QCoreApplication::applicationName();
        originalOrgName = QCoreApplication::organizationName();
        
        QCoreApplication::setApplicationName("ETIStreamAnalyser_Test");
        QCoreApplication::setOrganizationName("ETIStreamAnalyser_Test");
        
        configManager = new ConfigManager();
    }

    void TearDown() override 
    {
        delete configManager;
        configManager = nullptr;
        
        // Restore original application info
        QCoreApplication::setApplicationName(originalAppName);
        QCoreApplication::setOrganizationName(originalOrgName);
        
        delete tempDir;
        tempDir = nullptr;
    }

    // Helper methods
    void initializeConfigManager()
    {
        bool result = configManager->initialize();
        ASSERT_TRUE(result);
        ASSERT_TRUE(configManager->isInitialized());
    }

    void verifyDefaultValues()
    {
        EXPECT_EQ(configManager->getValue(ConfigManager::UPDATE_RATE).toInt(), 60);
        EXPECT_EQ(configManager->getValue(ConfigManager::REAL_TIME_MODE).toBool(), false);
        EXPECT_EQ(configManager->getValue(ConfigManager::AUDIO_MONITORING).toBool(), true);
        EXPECT_EQ(configManager->getValue(ConfigManager::ERROR_LOGGING).toString(), QString("Info"));
        EXPECT_FALSE(configManager->getValue(ConfigManager::LAST_FILE_PATH).toString().isEmpty());
    }

protected:
    ConfigManager* configManager;
    QTemporaryDir* tempDir;
    QString originalAppName;
    QString originalOrgName;
};

// Constructor and Initialization Tests
TEST_F(ConfigManagerTest, ConstructorInitializesCorrectly)
{
    EXPECT_FALSE(configManager->isInitialized());
    EXPECT_TRUE(configManager->getSettingsPath().isEmpty());
}

TEST_F(ConfigManagerTest, InitializationSucceeds)
{
    bool result = configManager->initialize();
    
    EXPECT_TRUE(result);
    EXPECT_TRUE(configManager->isInitialized());
    EXPECT_FALSE(configManager->getSettingsPath().isEmpty());
}

TEST_F(ConfigManagerTest, MultipleInitializationCallsSucceed)
{
    // First initialization
    bool result1 = configManager->initialize();
    EXPECT_TRUE(result1);
    EXPECT_TRUE(configManager->isInitialized());
    
    // Second initialization should also succeed
    bool result2 = configManager->initialize();
    EXPECT_TRUE(result2);
    EXPECT_TRUE(configManager->isInitialized());
}

TEST_F(ConfigManagerTest, DefaultValuesAreSetAfterInitialization)
{
    initializeConfigManager();
    verifyDefaultValues();
}

// Value Getting and Setting Tests
TEST_F(ConfigManagerTest, GetValueReturnsDefaultWhenNotInitialized)
{
    QVariant defaultValue = QString("test_default");
    QVariant result = configManager->getValue("test_key", defaultValue);
    
    EXPECT_EQ(result, defaultValue);
}

TEST_F(ConfigManagerTest, GetValueReturnsCorrectValue)
{
    initializeConfigManager();
    
    QString testKey = "test/value";
    QString testValue = "test_string_value";
    
    configManager->setValue(testKey, testValue);
    QVariant result = configManager->getValue(testKey);
    
    EXPECT_EQ(result.toString(), testValue);
}

TEST_F(ConfigManagerTest, GetValueReturnsDefaultForNonExistentKey)
{
    initializeConfigManager();
    
    QString defaultValue = "default_value";
    QVariant result = configManager->getValue("non_existent_key", defaultValue);
    
    EXPECT_EQ(result.toString(), defaultValue);
}

TEST_F(ConfigManagerTest, SetValueStoresCorrectly)
{
    initializeConfigManager();
    QSignalSpy configSpy(configManager, &ConfigManager::configurationChanged);
    
    QString testKey = "test/key";
    QString testValue = "test_value";
    
    configManager->setValue(testKey, testValue);
    
    EXPECT_EQ(configManager->getValue(testKey).toString(), testValue);
    EXPECT_EQ(configSpy.count(), 1);
    
    // Verify signal parameters
    auto signalArgs = configSpy.at(0);
    EXPECT_EQ(signalArgs.at(0).toString(), testKey);
    EXPECT_EQ(signalArgs.at(1).toString(), testValue);
}

TEST_F(ConfigManagerTest, SetValueDoesNothingWhenNotInitialized)
{
    QSignalSpy configSpy(configManager, &ConfigManager::configurationChanged);
    
    configManager->setValue("test_key", "test_value");
    
    EXPECT_EQ(configSpy.count(), 0);
}

TEST_F(ConfigManagerTest, SetSameValueDoesNotEmitSignal)
{
    initializeConfigManager();
    
    QString testKey = "test/key";
    QString testValue = "same_value";
    
    // Set initial value
    configManager->setValue(testKey, testValue);
    
    QSignalSpy configSpy(configManager, &ConfigManager::configurationChanged);
    
    // Set same value again
    configManager->setValue(testKey, testValue);
    
    EXPECT_EQ(configSpy.count(), 0);
}

// Data Type Tests
TEST_F(ConfigManagerTest, HandlesStringValues)
{
    initializeConfigManager();
    
    QString key = "string_test";
    QString value = "Hello, World!";
    
    configManager->setValue(key, value);
    EXPECT_EQ(configManager->getValue(key).toString(), value);
}

TEST_F(ConfigManagerTest, HandlesIntegerValues)
{
    initializeConfigManager();
    
    QString key = "int_test";
    int value = 42;
    
    configManager->setValue(key, value);
    EXPECT_EQ(configManager->getValue(key).toInt(), value);
}

TEST_F(ConfigManagerTest, HandlesBooleanValues)
{
    initializeConfigManager();
    
    QString key = "bool_test";
    bool value = true;
    
    configManager->setValue(key, value);
    EXPECT_EQ(configManager->getValue(key).toBool(), value);
}

TEST_F(ConfigManagerTest, HandlesDoubleValues)
{
    initializeConfigManager();
    
    QString key = "double_test";
    double value = 3.14159;
    
    configManager->setValue(key, value);
    EXPECT_DOUBLE_EQ(configManager->getValue(key).toDouble(), value);
}

TEST_F(ConfigManagerTest, HandlesQByteArrayValues)
{
    initializeConfigManager();
    
    QString key = "bytearray_test";
    QByteArray value = "binary_data_test";
    
    configManager->setValue(key, value);
    EXPECT_EQ(configManager->getValue(key).toByteArray(), value);
}

// Key Management Tests
TEST_F(ConfigManagerTest, ContainsReturnsTrueForExistingKey)
{
    initializeConfigManager();
    
    QString key = "existing_key";
    configManager->setValue(key, "value");
    
    EXPECT_TRUE(configManager->contains(key));
}

TEST_F(ConfigManagerTest, ContainsReturnsFalseForNonExistentKey)
{
    initializeConfigManager();
    
    EXPECT_FALSE(configManager->contains("non_existent_key"));
}

TEST_F(ConfigManagerTest, ContainsReturnsFalseWhenNotInitialized)
{
    EXPECT_FALSE(configManager->contains("any_key"));
}

TEST_F(ConfigManagerTest, RemoveDeletesKey)
{
    initializeConfigManager();
    
    QString key = "key_to_remove";
    configManager->setValue(key, "value");
    
    EXPECT_TRUE(configManager->contains(key));
    
    configManager->remove(key);
    
    EXPECT_FALSE(configManager->contains(key));
}

TEST_F(ConfigManagerTest, RemoveDoesNothingWhenNotInitialized)
{
    // Should not crash
    configManager->remove("any_key");
    EXPECT_FALSE(configManager->isInitialized());
}

TEST_F(ConfigManagerTest, ClearRemovesAllKeys)
{
    initializeConfigManager();
    
    // Set multiple values
    configManager->setValue("key1", "value1");
    configManager->setValue("key2", "value2");
    configManager->setValue("key3", "value3");
    
    EXPECT_TRUE(configManager->contains("key1"));
    EXPECT_TRUE(configManager->contains("key2"));
    EXPECT_TRUE(configManager->contains("key3"));
    
    configManager->clear();
    
    // Custom keys should be gone, but defaults should be restored
    EXPECT_FALSE(configManager->contains("key1"));
    EXPECT_FALSE(configManager->contains("key2"));
    EXPECT_FALSE(configManager->contains("key3"));
    
    // Default values should be present
    verifyDefaultValues();
}

// Persistence Tests
TEST_F(ConfigManagerTest, SyncEmitsConfigurationSavedSignal)
{
    initializeConfigManager();
    QSignalSpy syncSpy(configManager, &ConfigManager::configurationSaved);
    
    configManager->sync();
    
    EXPECT_EQ(syncSpy.count(), 1);
}

TEST_F(ConfigManagerTest, SyncDoesNothingWhenNotInitialized)
{
    QSignalSpy syncSpy(configManager, &ConfigManager::configurationSaved);
    
    configManager->sync();
    
    EXPECT_EQ(syncSpy.count(), 0);
}

TEST_F(ConfigManagerTest, ValuesPersistedAcrossInstances)
{
    // First instance
    {
        ConfigManager firstManager;
        firstManager.initialize();
        
        QString testKey = "persistence_test";
        QString testValue = "persistent_value";
        
        firstManager.setValue(testKey, testValue);
        firstManager.sync();
    }
    
    // Second instance should read the same value
    {
        ConfigManager secondManager;
        secondManager.initialize();
        
        QString retrievedValue = secondManager.getValue("persistence_test").toString();
        EXPECT_EQ(retrievedValue, QString("persistent_value"));
    }
}

// Predefined Configuration Keys Tests
TEST_F(ConfigManagerTest, PredefinedKeysAreCorrect)
{
    // Verify that predefined keys have expected values
    EXPECT_EQ(ConfigManager::WINDOW_GEOMETRY, QString("window/geometry"));
    EXPECT_EQ(ConfigManager::WINDOW_STATE, QString("window/state"));
    EXPECT_EQ(ConfigManager::LAST_FILE_PATH, QString("files/lastPath"));
    EXPECT_EQ(ConfigManager::UPDATE_RATE, QString("display/updateRate"));
    EXPECT_EQ(ConfigManager::REAL_TIME_MODE, QString("processing/realTimeMode"));
    EXPECT_EQ(ConfigManager::AUDIO_MONITORING, QString("audio/monitoring"));
    EXPECT_EQ(ConfigManager::ERROR_LOGGING, QString("logging/errorLevel"));
}

TEST_F(ConfigManagerTest, PredefinedKeysHaveDefaultValues)
{
    initializeConfigManager();
    
    EXPECT_TRUE(configManager->contains(ConfigManager::UPDATE_RATE));
    EXPECT_TRUE(configManager->contains(ConfigManager::REAL_TIME_MODE));
    EXPECT_TRUE(configManager->contains(ConfigManager::AUDIO_MONITORING));
    EXPECT_TRUE(configManager->contains(ConfigManager::ERROR_LOGGING));
    EXPECT_TRUE(configManager->contains(ConfigManager::LAST_FILE_PATH));
}

TEST_F(ConfigManagerTest, WindowGeometryCanBeStored)
{
    initializeConfigManager();
    
    QByteArray geometryData = "test_geometry_data";
    configManager->setValue(ConfigManager::WINDOW_GEOMETRY, geometryData);
    
    QByteArray retrieved = configManager->getValue(ConfigManager::WINDOW_GEOMETRY).toByteArray();
    EXPECT_EQ(retrieved, geometryData);
}

TEST_F(ConfigManagerTest, WindowStateCanBeStored)
{
    initializeConfigManager();
    
    QByteArray stateData = "test_window_state";
    configManager->setValue(ConfigManager::WINDOW_STATE, stateData);
    
    QByteArray retrieved = configManager->getValue(ConfigManager::WINDOW_STATE).toByteArray();
    EXPECT_EQ(retrieved, stateData);
}

// Application Configuration Tests
TEST_F(ConfigManagerTest, UpdateRateConfigurationWorks)
{
    initializeConfigManager();
    
    int newUpdateRate = 120;
    configManager->setValue(ConfigManager::UPDATE_RATE, newUpdateRate);
    
    EXPECT_EQ(configManager->getValue(ConfigManager::UPDATE_RATE).toInt(), newUpdateRate);
}

TEST_F(ConfigManagerTest, RealTimeModeConfigurationWorks)
{
    initializeConfigManager();
    
    bool realTimeMode = true;
    configManager->setValue(ConfigManager::REAL_TIME_MODE, realTimeMode);
    
    EXPECT_EQ(configManager->getValue(ConfigManager::REAL_TIME_MODE).toBool(), realTimeMode);
}

TEST_F(ConfigManagerTest, AudioMonitoringConfigurationWorks)
{
    initializeConfigManager();
    
    bool audioMonitoring = false;
    configManager->setValue(ConfigManager::AUDIO_MONITORING, audioMonitoring);
    
    EXPECT_EQ(configManager->getValue(ConfigManager::AUDIO_MONITORING).toBool(), audioMonitoring);
}

TEST_F(ConfigManagerTest, ErrorLoggingLevelWorks)
{
    initializeConfigManager();
    
    QString logLevel = "Debug";
    configManager->setValue(ConfigManager::ERROR_LOGGING, logLevel);
    
    EXPECT_EQ(configManager->getValue(ConfigManager::ERROR_LOGGING).toString(), logLevel);
}

TEST_F(ConfigManagerTest, LastFilePathWorks)
{
    initializeConfigManager();
    
    QString newPath = "/test/path/to/file.eti";
    configManager->setValue(ConfigManager::LAST_FILE_PATH, newPath);
    
    EXPECT_EQ(configManager->getValue(ConfigManager::LAST_FILE_PATH).toString(), newPath);
}

// Signal Tests
TEST_F(ConfigManagerTest, ConfigurationChangedSignalEmitsCorrectData)
{
    initializeConfigManager();
    QSignalSpy configSpy(configManager, &ConfigManager::configurationChanged);
    
    QString testKey = "signal_test_key";
    QString testValue = "signal_test_value";
    
    configManager->setValue(testKey, testValue);
    
    EXPECT_EQ(configSpy.count(), 1);
    
    auto signalArgs = configSpy.at(0);
    EXPECT_EQ(signalArgs.at(0).toString(), testKey);
    EXPECT_EQ(signalArgs.at(1).toString(), testValue);
}

TEST_F(ConfigManagerTest, MultipleChangesEmitMultipleSignals)
{
    initializeConfigManager();
    QSignalSpy configSpy(configManager, &ConfigManager::configurationChanged);
    
    configManager->setValue("key1", "value1");
    configManager->setValue("key2", "value2");
    configManager->setValue("key3", "value3");
    
    EXPECT_EQ(configSpy.count(), 3);
}

// Error Handling Tests
TEST_F(ConfigManagerTest, HandlesEmptyKeys)
{
    initializeConfigManager();
    
    QString emptyKey = "";
    QString testValue = "test_value";
    
    configManager->setValue(emptyKey, testValue);
    QVariant result = configManager->getValue(emptyKey);
    
    EXPECT_EQ(result.toString(), testValue);
}

TEST_F(ConfigManagerTest, HandlesSpecialCharactersInKeys)
{
    initializeConfigManager();
    
    QString specialKey = "test/key with spaces & symbols!@#$%^&*()";
    QString testValue = "special_value";
    
    configManager->setValue(specialKey, testValue);
    QVariant result = configManager->getValue(specialKey);
    
    EXPECT_EQ(result.toString(), testValue);
}

TEST_F(ConfigManagerTest, HandlesNullVariants)
{
    initializeConfigManager();
    
    QString testKey = "null_test";
    QVariant nullVariant;
    
    configManager->setValue(testKey, nullVariant);
    QVariant result = configManager->getValue(testKey);
    
    EXPECT_FALSE(result.isValid());
}

// Settings Path Tests
TEST_F(ConfigManagerTest, SettingsPathIsValidAfterInitialization)
{
    initializeConfigManager();
    
    QString settingsPath = configManager->getSettingsPath();
    
    EXPECT_FALSE(settingsPath.isEmpty());
    EXPECT_TRUE(settingsPath.contains("ETIStreamAnalyser_Test"));
}

TEST_F(ConfigManagerTest, SettingsPathIsEmptyBeforeInitialization)
{
    QString settingsPath = configManager->getSettingsPath();
    EXPECT_TRUE(settingsPath.isEmpty());
}

// Destructor Tests
TEST_F(ConfigManagerTest, DestructorSyncsSettings)
{
    ConfigManager* tempManager = new ConfigManager();
    tempManager->initialize();
    
    QSignalSpy syncSpy(tempManager, &ConfigManager::configurationSaved);
    
    tempManager->setValue("destructor_test", "test_value");
    
    // Delete should sync settings
    delete tempManager;
    
    // Note: We can't easily test the sync in destructor with signals
    // since the object is being destroyed, but this test ensures
    // the destructor doesn't crash
}

// Complex Configuration Scenarios
TEST_F(ConfigManagerTest, HandlesNestedConfigurationKeys)
{
    initializeConfigManager();
    
    configManager->setValue("level1/level2/level3/key", "nested_value");
    configManager->setValue("level1/level2/other_key", "other_value");
    configManager->setValue("level1/different_key", "different_value");
    
    EXPECT_EQ(configManager->getValue("level1/level2/level3/key").toString(), QString("nested_value"));
    EXPECT_EQ(configManager->getValue("level1/level2/other_key").toString(), QString("other_value"));
    EXPECT_EQ(configManager->getValue("level1/different_key").toString(), QString("different_value"));
}

TEST_F(ConfigManagerTest, SupportsLargeDataValues)
{
    initializeConfigManager();
    
    // Create large string (1MB)
    QString largeValue(1024 * 1024, 'A');
    QString testKey = "large_data_test";
    
    configManager->setValue(testKey, largeValue);
    QString retrievedValue = configManager->getValue(testKey).toString();
    
    EXPECT_EQ(retrievedValue.size(), largeValue.size());
    EXPECT_EQ(retrievedValue, largeValue);
}

// Integration Tests
TEST_F(ConfigManagerTest, CompleteWorkflowTest)
{
    // Test complete configuration management workflow
    
    // 1. Initialize
    initializeConfigManager();
    
    // 2. Set various configuration values
    configManager->setValue(ConfigManager::UPDATE_RATE, 120);
    configManager->setValue(ConfigManager::REAL_TIME_MODE, true);
    configManager->setValue(ConfigManager::AUDIO_MONITORING, false);
    configManager->setValue("custom/setting", "custom_value");
    
    // 3. Verify values
    EXPECT_EQ(configManager->getValue(ConfigManager::UPDATE_RATE).toInt(), 120);
    EXPECT_EQ(configManager->getValue(ConfigManager::REAL_TIME_MODE).toBool(), true);
    EXPECT_EQ(configManager->getValue(ConfigManager::AUDIO_MONITORING).toBool(), false);
    EXPECT_EQ(configManager->getValue("custom/setting").toString(), QString("custom_value"));
    
    // 4. Sync to disk
    QSignalSpy syncSpy(configManager, &ConfigManager::configurationSaved);
    configManager->sync();
    EXPECT_EQ(syncSpy.count(), 1);
    
    // 5. Clear and verify defaults are restored
    configManager->clear();
    verifyDefaultValues();
    EXPECT_FALSE(configManager->contains("custom/setting"));
}

// Thread Safety Note
// Qt's QSettings is generally thread-safe for reading, but writing should be done
// from a single thread or with proper synchronization. These tests run in a single
// thread, so they don't test thread safety explicitly.
TEST_F(ConfigManagerTest, QObjectFunctionalityWorks)
{
    // Verify Qt object functionality
    initializeConfigManager();
    
    EXPECT_NE(configManager->metaObject(), nullptr);
    EXPECT_TRUE(configManager->inherits("QObject"));
    
    // Test that signals are properly connected and working
    QSignalSpy configSpy(configManager, &ConfigManager::configurationChanged);
    EXPECT_TRUE(configSpy.isValid());
    
    configManager->setValue("qt_test", "value");
    EXPECT_EQ(configSpy.count(), 1);
}