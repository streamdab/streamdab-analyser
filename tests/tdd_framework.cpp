/**
 * @file tdd_framework.cpp
 * @brief Implementation of the comprehensive TDD framework
 * 
 * @author TDD Lead Agent
 * @date 2025-09-22
 * @copyright StreamDAB Analyser Project
 */

#include "tdd_framework.h"
#include <QDateTime>
#include <QTextStream>
#include <QDir>
#include <QStandardPaths>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QDebug>

namespace TDD {

// ETITestFramework implementation
QByteArray ETITestFramework::generateValidETIFrame(int frameSize) {
    QByteArray frame(frameSize, 0);
    
    // ETI Frame Header (ETSI EN 300 799 Section 5.2)
    frame[0] = 0x49;  // Frame sync byte 1 ('I')
    frame[1] = 0x4E;  // Frame sync byte 2 ('N')
    frame[2] = 0x53;  // Frame sync byte 3 ('S')
    frame[3] = 0x54;  // Frame sync byte 4 ('T')
    
    // Frame Number (24 bits)
    frame[4] = 0x00;  // Frame number high byte
    frame[5] = 0x00;  // Frame number mid byte
    frame[6] = 0x01;  // Frame number low byte
    
    // Ensemble ID (16 bits)
    frame[7] = 0x10;  // Ensemble ID high byte
    frame[8] = 0x01;  // Ensemble ID low byte
    
    // Mode and configuration
    frame[9] = 0x01;  // Mode I (ETSI EN 300 401)
    frame[10] = 0x00; // Reserved
    frame[11] = 0x00; // Reserved
    
    // Fill rest with valid pattern
    for (int i = 12; i < frameSize; ++i) {
        frame[i] = static_cast<char>((i * 7) % 256); // Pseudo-random pattern
    }
    
    return frame;
}

QByteArray ETITestFramework::generateInvalidETIFrame(const QString& corruptionType) {
    QByteArray frame = generateValidETIFrame();
    
    if (corruptionType == "invalid_sync") {
        frame[0] = 0xFF; // Corrupt sync pattern
    } else if (corruptionType == "invalid_size") {
        frame.resize(100); // Invalid frame size
    } else if (corruptionType == "crc_error") {
        // Corrupt data but keep header valid
        frame[frame.size() - 1] = 0xFF;
    } else if (corruptionType == "invalid_ensemble_id") {
        frame[7] = 0x00; // Invalid ensemble ID
        frame[8] = 0x00;
    }
    
    return frame;
}

bool ETITestFramework::validateETIFrameStructure(const QByteArray& frameData) {
    if (frameData.size() != 6144) {
        qDebug() << "Invalid frame size:" << frameData.size() << "(expected 6144)";
        return false;
    }
    
    // Check sync pattern (ETSI EN 300 799)
    if (frameData[0] != 0x49 || frameData[1] != 0x4E || 
        frameData[2] != 0x53 || frameData[3] != 0x54) {
        qDebug() << "Invalid sync pattern";
        return false;
    }
    
    // Basic frame structure validation
    quint32 frameNumber = (static_cast<quint8>(frameData[4]) << 16) |
                         (static_cast<quint8>(frameData[5]) << 8) |
                         static_cast<quint8>(frameData[6]);
    
    if (frameNumber > 499999) { // Max frame number in 24-bit field
        qDebug() << "Invalid frame number:" << frameNumber;
        return false;
    }
    
    return true;
}

QVariantMap ETITestFramework::generateTestEnsemble() {
    QVariantMap ensemble;
    ensemble["ensembleId"] = 0x1001;
    ensemble["ensembleLabel"] = "Test Ensemble";
    ensemble["countryId"] = 0x0C; // Germany country code
    ensemble["extendedCountryCode"] = 0x02;
    ensemble["serviceCount"] = 3;
    
    QVariantList services;
    services.append(generateTestService("audio"));
    services.append(generateTestService("audio"));
    services.append(generateTestService("data"));
    ensemble["services"] = services;
    
    return ensemble;
}

QVariantMap ETITestFramework::generateTestService(const QString& serviceType) {
    QVariantMap service;
    static int serviceCounter = 1;
    
    service["serviceId"] = 0x1000 + serviceCounter;
    service["serviceLabel"] = QString("Test Service %1").arg(serviceCounter);
    service["serviceType"] = serviceType;
    
    if (serviceType == "audio") {
        service["bitrate"] = 128; // kbps
        service["protectionLevel"] = 2;
        service["codecType"] = "DAB+";
    } else if (serviceType == "data") {
        service["bitrate"] = 32; // kbps
        service["protectionLevel"] = 1;
        service["dataType"] = "MOT";
    }
    
    service["subchannelId"] = serviceCounter;
    service["startAddress"] = serviceCounter * 72; // CU units
    
    serviceCounter++;
    return service;
}

// TestReporter implementation
TestReporter& TestReporter::instance() {
    static TestReporter instance;
    return instance;
}

void TestReporter::recordTestStart(const QString& testName, Category category) {
    TestResult result;
    result.name = testName;
    result.category = category;
    result.passed = false; // Will be updated in recordTestEnd
    result.durationMs = QDateTime::currentMSecsSinceEpoch();
    
    qDebug() << "Starting test:" << testName;
}

void TestReporter::recordTestEnd(const QString& testName, bool passed, const QString& message) {
    // Find the corresponding test start record
    for (auto& result : m_results) {
        if (result.name == testName) {
            result.passed = passed;
            result.message = message;
            result.durationMs = QDateTime::currentMSecsSinceEpoch() - result.durationMs;
            
            QString status = passed ? "PASSED" : "FAILED";
            qDebug() << "Test" << testName << status << "in" << result.durationMs << "ms";
            return;
        }
    }
    
    // If not found, create new record (for tests that didn't call recordTestStart)
    TestResult result;
    result.name = testName;
    result.category = Category::UNIT; // Default category
    result.passed = passed;
    result.message = message;
    result.durationMs = 0;
    m_results.append(result);
}

void TestReporter::recordCoverage(const QString& file, int linesTotal, int linesCovered) {
    CoverageData& data = m_coverage[file];
    data.linesTotal = linesTotal;
    data.linesCovered = linesCovered;
    
    double percentage = (linesTotal > 0) ? (double(linesCovered) / double(linesTotal)) * 100.0 : 0.0;
    qDebug() << "Coverage for" << file << ":" << linesCovered << "/" << linesTotal 
             << "(" << QString::number(percentage, 'f', 1) << "%)";
}

bool TestReporter::checkCoverageRequirement(double minPercentage) {
    if (m_coverage.isEmpty()) {
        qWarning() << "No coverage data available";
        return false;
    }
    
    int totalLines = 0;
    int totalCovered = 0;
    
    for (auto it = m_coverage.begin(); it != m_coverage.end(); ++it) {
        totalLines += it.value().linesTotal;
        totalCovered += it.value().linesCovered;
    }
    
    double overallPercentage = (totalLines > 0) ? (double(totalCovered) / double(totalLines)) * 100.0 : 0.0;
    
    qDebug() << "Overall coverage:" << totalCovered << "/" << totalLines 
             << "(" << QString::number(overallPercentage, 'f', 1) << "%)";
    qDebug() << "Required minimum:" << QString::number(minPercentage, 'f', 1) << "%";
    
    bool passed = overallPercentage >= minPercentage;
    if (!passed) {
        qCritical() << "Coverage requirement FAILED: " << QString::number(overallPercentage, 'f', 1) 
                   << "% < " << QString::number(minPercentage, 'f', 1) << "%";
    }
    
    return passed;
}

void TestReporter::enforceTDDCompliance() {
    qDebug() << "\n=== TDD Compliance Check ===";
    
    int totalTests = m_results.size();
    int passedTests = 0;
    int failedTests = 0;
    
    // Categorize test results
    QMap<Category, int> categoryPassed;
    QMap<Category, int> categoryTotal;
    
    for (const auto& result : m_results) {
        categoryTotal[result.category]++;
        if (result.passed) {
            passedTests++;
            categoryPassed[result.category]++;
        } else {
            failedTests++;
        }
    }
    
    qDebug() << "Total tests:" << totalTests;
    qDebug() << "Passed tests:" << passedTests;
    qDebug() << "Failed tests:" << failedTests;
    
    if (totalTests > 0) {
        double passRate = (double(passedTests) / double(totalTests)) * 100.0;
        qDebug() << "Pass rate:" << QString::number(passRate, 'f', 1) << "%";
        
        if (passRate < 80.0) {
            qCritical() << "TDD COMPLIANCE FAILED: Pass rate below 80%";
        }
    }
    
    // Check category-specific requirements
    auto categories = {Category::UNIT, Category::INTEGRATION, Category::E2E, Category::GUI, Category::ETSI};
    for (Category cat : categories) {
        int total = categoryTotal[cat];
        int passed = categoryPassed[cat];
        
        if (total > 0) {
            double catPassRate = (double(passed) / double(total)) * 100.0;
            QString catName;
            switch (cat) {
                case Category::UNIT: catName = "UNIT"; break;
                case Category::INTEGRATION: catName = "INTEGRATION"; break;
                case Category::E2E: catName = "E2E"; break;
                case Category::GUI: catName = "GUI"; break;
                case Category::ETSI: catName = "ETSI"; break;
                default: catName = "UNKNOWN"; break;
            }
            qDebug() << catName << "tests:" << passed << "/" << total 
                     << "(" << QString::number(catPassRate, 'f', 1) << "%)";
        }
    }
    
    qDebug() << "=== TDD Compliance Check Complete ===\n";
}

void TestReporter::generateReport() {
    QString reportDir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + "/test_reports";
    QDir().mkpath(reportDir);
    
    QString timestamp = QDateTime::currentDateTime().toString("yyyyMMdd_hhmmss");
    QString reportFile = reportDir + "/tdd_report_" + timestamp + ".json";
    
    QJsonObject report;
    report["timestamp"] = QDateTime::currentDateTime().toString(Qt::ISODate);
    report["totalTests"] = m_results.size();
    
    QJsonArray testsArray;
    for (const auto& result : m_results) {
        QJsonObject testObj;
        testObj["name"] = result.name;
        testObj["passed"] = result.passed;
        testObj["message"] = result.message;
        testObj["duration"] = result.durationMs;
        
        QString categoryStr;
        switch (result.category) {
            case Category::UNIT: categoryStr = "UNIT"; break;
            case Category::INTEGRATION: categoryStr = "INTEGRATION"; break;
            case Category::E2E: categoryStr = "E2E"; break;
            case Category::GUI: categoryStr = "GUI"; break;
            case Category::ETSI: categoryStr = "ETSI"; break;
            default: categoryStr = "UNKNOWN"; break;
        }
        testObj["category"] = categoryStr;
        
        testsArray.append(testObj);
    }
    report["tests"] = testsArray;
    
    // Coverage data
    QJsonObject coverageObj;
    int totalLines = 0;
    int totalCovered = 0;
    
    for (auto it = m_coverage.begin(); it != m_coverage.end(); ++it) {
        QJsonObject fileObj;
        fileObj["total"] = it.value().linesTotal;
        fileObj["covered"] = it.value().linesCovered;
        
        totalLines += it.value().linesTotal;
        totalCovered += it.value().linesCovered;
        
        coverageObj[it.key()] = fileObj;
    }
    
    QJsonObject coverageSummary;
    coverageSummary["totalLines"] = totalLines;
    coverageSummary["coveredLines"] = totalCovered;
    coverageSummary["percentage"] = (totalLines > 0) ? (double(totalCovered) / double(totalLines)) * 100.0 : 0.0;
    
    report["coverage"] = coverageObj;
    report["coverageSummary"] = coverageSummary;
    
    // Write report to file
    QFile file(reportFile);
    if (file.open(QIODevice::WriteOnly)) {
        QJsonDocument doc(report);
        file.write(doc.toJson());
        qDebug() << "Test report written to:" << reportFile;
    } else {
        qWarning() << "Failed to write test report to:" << reportFile;
    }
}

} // namespace TDD