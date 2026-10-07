/**
 * @file style_compliance_test.cpp
 * @brief TDD Style Compliance Test Suite - Achieving 10.0/10.0 Perfect Score
 *
 * This file implements comprehensive style compliance testing using TDD/AAA methodology
 * to systematically achieve perfect 10.0/10.0 style compliance scoring.
 */

#include <gtest/gtest.h>
#include <QDir>
#include <QFileInfo>
#include <QTextStream>
#include <QRegularExpression>
#include <QStringList>

/**
 * @class StyleComplianceTest
 * @brief TDD test fixture for systematic style compliance validation
 * 
 * Uses AAA (Arrange-Act-Assert) pattern to validate:
 * - Layer-based naming conventions
 * - File structure compliance
 * - Code formatting standards
 * - Qt6 + C++20 modernization
 */
class StyleComplianceTest : public ::testing::Test {
protected:
    void SetUp() override {
        projectRoot = QDir::currentPath();
        if (!projectRoot.endsWith("streamdab-analyser")) {
            // Navigate to project root if we're in build directory
            while (!QDir(projectRoot).exists("src") && 
                   !QDir(projectRoot).exists("CMakeLists.txt")) {
                QDir dir(projectRoot);
                if (!dir.cdUp()) break;
                projectRoot = dir.absolutePath();
            }
        }
    }

    QString projectRoot;
    
    /**
     * @brief Get all source files for specific layer
     */
    QStringList getLayerFiles(const QString& layer) {
        QDir layerDir(projectRoot + "/src/" + layer);
        QStringList filters;
        filters << "*.h" << "*.hpp" << "*.cpp";
        return layerDir.entryList(filters, QDir::Files, QDir::Name);
    }
    
    /**
     * @brief Read file content for analysis
     */
    QString readFileContent(const QString& filePath) {
        QFile file(filePath);
        if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            return QString();
        }
        QTextStream in(&file);
        return in.readAll();
    }
    
    /**
     * @brief Check naming convention violations
     */
    QStringList checkNamingViolations(const QString& layer, const QString& filePath) {
        QStringList violations;
        QString content = readFileContent(filePath);
        
        // Remove comments to avoid false positives
        QString cleanContent = content;
        cleanContent.remove(QRegularExpression("/\\*[^*]*\\*+(?:[^/*][^*]*\\*+)*/"));  // /* */ comments
        cleanContent.remove(QRegularExpression("//.*"));  // // comments
        
        // Define expected conventions per layer - match actual class declarations
        QRegularExpression classPattern("^\\s*class\\s+(\\w+)\\s*[:{;]", QRegularExpression::MultilineOption);
        QRegularExpressionMatchIterator matches = classPattern.globalMatch(cleanContent);
        
        while (matches.hasNext()) {
            QRegularExpressionMatch match = matches.next();
            QString className = match.captured(1);
            
            // Skip common C++ keywords and Qt macros
            if (className == "Q_OBJECT" || className == "Q_GADGET" || 
                className == "public" || className == "private" || className == "protected") {
                continue;
            }
            
            if (layer == "gui") {
                // UI Layer should use camelCase (Qt convention)
                if (!isQtCamelCase(className)) {
                    violations << QString("Class '%1' in GUI layer should use camelCase").arg(className);
                }
            } else if (layer == "core" || layer == "network") {
                // Core/Network layers should use snake_case or PascalCase
                if (!isSnakeCaseOrPascalCase(className)) {
                    violations << QString("Class '%1' in %2 layer should use snake_case or PascalCase").arg(className, layer);
                }
            }
        }
        
        return violations;
    }
    
private:
    bool isQtCamelCase(const QString& name) {
        // Qt convention: PascalCase for classes
        return name.at(0).isUpper() && !name.contains('_');
    }
    
    bool isSnakeCaseOrPascalCase(const QString& name) {
        // Allow both snake_case and PascalCase for core/network
        return name.contains('_') || (name.at(0).isUpper() && !name.contains('_'));
    }
};

/**
 * RED PHASE TEST: Naming Convention Compliance
 * This test MUST FAIL initially to follow TDD methodology
 */
TEST_F(StyleComplianceTest, LayerBasedNamingConventionComplianceAAA) {
    // Arrange
    QStringList layers = {"gui", "core", "network"};
    QStringList allViolations;
    
    // Act
    for (const QString& layer : layers) {
        QStringList files = getLayerFiles(layer);
        for (const QString& file : files) {
            if (file.endsWith(".h") || file.endsWith(".hpp")) {
                QString fullPath = projectRoot + "/src/" + layer + "/" + file;
                QStringList violations = checkNamingViolations(layer, fullPath);
                allViolations.append(violations);
            }
        }
    }
    
    // Assert - This MUST FAIL initially (RED phase)
    ASSERT_TRUE(allViolations.isEmpty()) 
        << "Naming convention violations found: " 
        << allViolations.join("; ").toStdString();
}

/**
 * RED PHASE TEST: File Extension Compliance
 * Core layer should use .hpp, GUI layer should use .h
 */
TEST_F(StyleComplianceTest, FileExtensionComplianceAAA) {
    // Arrange
    QStringList coreFiles = getLayerFiles("core");
    QStringList guiFiles = getLayerFiles("gui");
    QStringList violations;
    
    // Act - Check core files use .hpp
    for (const QString& file : coreFiles) {
        if (file.endsWith(".h") && !file.startsWith("ui_")) {
            violations << QString("Core file '%1' should use .hpp extension").arg(file);
        }
    }
    
    // Act - Check GUI files use .h (Qt convention)
    for (const QString& file : guiFiles) {
        if (file.endsWith(".hpp")) {
            violations << QString("GUI file '%1' should use .h extension (Qt convention)").arg(file);
        }
    }
    
    // Assert - This MUST FAIL initially (RED phase)
    ASSERT_TRUE(violations.isEmpty()) 
        << "File extension violations found: " 
        << violations.join("; ").toStdString();
}

/**
 * RED PHASE TEST: Include Organization Compliance
 * Includes should be organized: system → Qt → project
 */
TEST_F(StyleComplianceTest, IncludeOrganizationComplianceAAA) {
    // Arrange
    QStringList allFiles;
    QStringList layers = {"gui", "core", "network"};
    
    for (const QString& layer : layers) {
        QStringList files = getLayerFiles(layer);
        for (const QString& file : files) {
            if (file.endsWith(".cpp") || file.endsWith(".h") || file.endsWith(".hpp")) {
                allFiles << projectRoot + "/src/" + layer + "/" + file;
            }
        }
    }
    
    QStringList violations;
    
    // Act
    for (const QString& filePath : allFiles) {
        QString content = readFileContent(filePath);
        QStringList lines = content.split('\n');
        
        bool foundSystemInclude = false;
        bool foundQtInclude = false;
        bool foundProjectInclude = false;
        
        for (const QString& line : lines) {
            QString trimmed = line.trimmed();
            if (trimmed.startsWith("#include")) {
                if (trimmed.contains("<") && !trimmed.contains("Q")) {
                    foundSystemInclude = true;
                    if (foundQtInclude || foundProjectInclude) {
                        violations << QString("System include after Qt/project include in %1").arg(QFileInfo(filePath).fileName());
                    }
                } else if (trimmed.contains("Q")) {
                    foundQtInclude = true;
                    if (foundProjectInclude) {
                        violations << QString("Qt include after project include in %1").arg(QFileInfo(filePath).fileName());
                    }
                } else if (trimmed.contains("\"")) {
                    foundProjectInclude = true;
                }
            }
        }
    }
    
    // Assert - This MUST FAIL initially (RED phase)
    ASSERT_TRUE(violations.isEmpty()) 
        << "Include organization violations found: " 
        << violations.join("; ").toStdString();
}

/**
 * RED PHASE TEST: Brace Style Compliance
 * Functions should use Allman style, control flow same-line
 */
TEST_F(StyleComplianceTest, BraceStyleComplianceAAA) {
    // Arrange
    QStringList allFiles;
    QStringList layers = {"gui", "core", "network"};
    
    for (const QString& layer : layers) {
        QStringList files = getLayerFiles(layer);
        for (const QString& file : files) {
            if (file.endsWith(".cpp") || file.endsWith(".h") || file.endsWith(".hpp")) {
                allFiles << projectRoot + "/src/" + layer + "/" + file;
            }
        }
    }
    
    QStringList violations;
    
    // Act - Check function brace style (should be Allman)
    for (const QString& filePath : allFiles) {
        QString content = readFileContent(filePath);
        QStringList lines = content.split('\n');
        
        for (int i = 0; i < lines.size(); ++i) {
            QString line = lines[i].trimmed();
            
            // Function detection (simplified)
            if (line.contains("(") && line.contains(")") && 
                !line.startsWith("if") && !line.startsWith("for") && 
                !line.startsWith("while") && !line.startsWith("//") &&
                !line.startsWith("*") && line.endsWith("{")) {
                violations << QString("Function brace should be on new line (Allman style) in %1:%2")
                             .arg(QFileInfo(filePath).fileName()).arg(i + 1);
            }
        }
    }
    
    // Assert - This MUST FAIL initially (RED phase)
    ASSERT_TRUE(violations.isEmpty()) 
        << "Brace style violations found: " 
        << violations.join("; ").toStdString();
}

/**
 * RED PHASE TEST: Pointer Alignment Compliance
 * Pointers should be left-aligned: int* ptr
 */
TEST_F(StyleComplianceTest, PointerAlignmentComplianceAAA) {
    // Arrange
    QStringList allFiles;
    QStringList layers = {"gui", "core", "network"};
    
    for (const QString& layer : layers) {
        QStringList files = getLayerFiles(layer);
        for (const QString& file : files) {
            if (file.endsWith(".cpp") || file.endsWith(".h") || file.endsWith(".hpp")) {
                allFiles << projectRoot + "/src/" + layer + "/" + file;
            }
        }
    }
    
    QStringList violations;
    
    // Act - Check pointer alignment
    QRegularExpression rightAlignedPointer("\\w+\\s+\\*\\w+");
    QRegularExpression centerAlignedPointer("\\w+\\s+\\*\\s+\\w+");
    
    for (const QString& filePath : allFiles) {
        QString content = readFileContent(filePath);
        QStringList lines = content.split('\n');
        
        for (int i = 0; i < lines.size(); ++i) {
            QString line = lines[i];
            
            if (rightAlignedPointer.match(line).hasMatch()) {
                violations << QString("Right-aligned pointer found in %1:%2 (should be left-aligned)")
                             .arg(QFileInfo(filePath).fileName()).arg(i + 1);
            }
            
            if (centerAlignedPointer.match(line).hasMatch()) {
                violations << QString("Center-aligned pointer found in %1:%2 (should be left-aligned)")
                             .arg(QFileInfo(filePath).fileName()).arg(i + 1);
            }
        }
    }
    
    // Assert - This MUST FAIL initially (RED phase)
    ASSERT_TRUE(violations.isEmpty()) 
        << "Pointer alignment violations found: " 
        << violations.join("; ").toStdString();
}

/**
 * RED PHASE TEST: C++20 Modernization Compliance
 * Check for modern C++20 features usage
 */
TEST_F(StyleComplianceTest, Cpp20ModernizationComplianceAAA) {
    // Arrange
    QStringList coreFiles;
    QStringList files = getLayerFiles("core");
    
    for (const QString& file : files) {
        if (file.endsWith(".cpp") || file.endsWith(".hpp")) {
            coreFiles << projectRoot + "/src/core/" + file;
        }
    }
    
    QStringList violations;
    
    // Act - Check for modern C++20 usage opportunities
    for (const QString& filePath : coreFiles) {
        QString content = readFileContent(filePath);
        
        // Check for raw loops that could be range-based
        if (content.contains("for (int i = 0; i <") || 
            content.contains("for (unsigned i = 0; i <")) {
            violations << QString("Consider range-based for loop in %1")
                         .arg(QFileInfo(filePath).fileName());
        }
        
        // Check for old-style function declarations
        if (content.contains("typedef") && content.contains("function")) {
            violations << QString("Consider std::function or auto in %1")
                         .arg(QFileInfo(filePath).fileName());
        }
    }
    
    // Assert - This MUST FAIL initially (RED phase)
    ASSERT_TRUE(violations.isEmpty()) 
        << "C++20 modernization opportunities found: " 
        << violations.join("; ").toStdString();
}