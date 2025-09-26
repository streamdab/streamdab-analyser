# Qt6 + C++20 Project Style Guide

## Table of Contents
1. [Overview](#overview)
2. [Naming Conventions](#naming-conventions)
3. [Code Organization](#code-organization)
4. [Formatting Rules](#formatting-rules)
5. [Qt-Specific Guidelines](#qt-specific-guidelines)
6. [C++20 Modern Features](#cpp20-modern-features)
7. [File Structure](#file-structure)
8. [Documentation Standards](#documentation-standards)
9. [Tooling Configuration](#tooling-configuration)
10. [Examples](#examples)

## Overview

This style guide defines coding standards for projects that combine Qt6 framework with modern C++20 features. The goal is to maintain consistency while respecting both Qt conventions and modern C++ best practices.

### Core Principles
- **Consistency**: Choose one style per module/layer and stick to it
- **Clarity**: Code should be self-documenting
- **Modern**: Leverage C++20 features appropriately
- **Qt Integration**: Respect Qt's conventions where they make sense

## Naming Conventions

### Layer-Based Approach

#### Qt UI Layer (camelCase)
Use Qt's traditional naming conventions for UI-related code:

```cpp
// Classes
class MainWindow : public QMainWindow {};
class UserDialog : public QDialog {};

// Methods and variables
void setText(const QString& text);
QString userName;
QTimer* connectionTimer;

// Member variables
QString m_currentUser;
QList<QWidget*> m_widgets;
```

#### C++ Core Layer (snake_case)
Use standard C++20 conventions for business logic:

```cpp
// Classes (choose one: PascalCase or snake_case consistently)
class UserManager {};  // or user_manager
class DatabaseConnection {};  // or database_connection

// Functions and variables
void process_user_data();
std::string user_name;
auto connection_pool = std::make_shared<ConnectionPool>();

// Member variables
std::string user_name_;
std::vector<User> users_;
```

#### Adapter Layer
Choose the style that matches the primary consumer of the adapter.

### Specific Naming Rules

#### Constants
```cpp
// Macros (avoid when possible)
#define MAX_BUFFER_SIZE 1024

// Qt constants
static constexpr int DefaultTimeout = 5000;

// C++ constants
static constexpr int default_timeout = 5000;
```

#### Enums
```cpp
// Qt style
enum class ConnectionState {
    Disconnected,
    Connecting,
    Connected
};

// C++ style
enum class connection_state {
    disconnected,
    connecting,
    connected
};
```

## Code Organization

### Directory Structure
```
project/
├── src/
│   ├── ui/           # Qt widgets, UI classes (Qt naming)
│   ├── core/         # Business logic (C++20 naming)
│   ├── models/       # Data models (consistent within)
│   ├── adapters/     # UI-Core bridges
│   └── utils/        # Helper functions
├── include/
│   └── [mirror src structure]
├── resources/
│   └── qml/          # QML files
├── tests/
└── docs/
```

### File Naming
- Header files: `.hpp` (C++) or `.h` (Qt classes)
- Implementation files: `.cpp`
- Qt UI files: `.ui`
- QML files: `.qml`
- Resource files: `.qrc`

### Include Organization
```cpp
// System headers
#include <memory>
#include <string>
#include <vector>

// Qt headers
#include <QApplication>
#include <QMainWindow>
#include <QString>

// Project headers
#include "core/user_manager.hpp"
#include "ui/MainWindow.h"
```

## Formatting Rules

### Indentation and Spacing
- Use 4 spaces (no tabs)
- Max line length: 100 characters
- Space after control flow keywords: `if (`, `for (`, `while (`

### Braces
```cpp
// Function braces - next line
void function_name()
{
    // implementation
}

// Control flow - same line
if (condition) {
    // code
} else {
    // code
}

// Class braces - next line
class ClassName
{
public:
    // members
};
```

### Pointer and Reference Alignment
```cpp
// Preferred
int* ptr;
int& ref;

// Acceptable (be consistent)
int *ptr;
int &ref;
```

## Qt-Specific Guidelines

### Signals and Slots
```cpp
// Signals - past tense, camelCase
signals:
    void userLoggedIn(const QString& username);
    void dataChanged();
    void errorOccurred(const QString& message);

// Slots - action-oriented, camelCase
private slots:
    void onButtonClicked();
    void handleUserInput();
    void updateDisplay();
```

### Qt Properties
```cpp
class CustomWidget : public QWidget
{
    Q_OBJECT
    Q_PROPERTY(QString title READ title WRITE setTitle NOTIFY titleChanged)
    
public:
    QString title() const { return m_title; }
    void setTitle(const QString& title);
    
signals:
    void titleChanged();
    
private:
    QString m_title;
};
```

### Memory Management
```cpp
// Prefer Qt parent-child hierarchy
auto* widget = new QWidget(parent);  // parent manages lifetime

// Use smart pointers for non-QObject classes
auto manager = std::make_unique<UserManager>();

// Qt containers for Qt types
QList<QString> names;
QHash<QString, QVariant> properties;

// STL containers for value types
std::vector<int> numbers;
std::unordered_map<std::string, User> users;
```

## C++20 Modern Features

### Concepts
```cpp
template<typename T>
concept Drawable = requires(T t) {
    t.draw();
};

template<Drawable T>
void render(const T& object) {
    object.draw();
}
```

### Ranges
```cpp
#include <ranges>

// Filter and transform user data
auto active_usernames = users 
    | std::views::filter([](const User& u) { return u.is_active(); })
    | std::views::transform([](const User& u) { return u.name(); });
```

### Coroutines (when appropriate)
```cpp
#include <coroutine>

// For async operations
Task<QString> fetchUserDataAsync(int userId);
```

### Structured Bindings
```cpp
// With std::pair/tuple
auto [success, message] = validate_user(user_data);

// With custom types
auto [x, y, width, height] = get_window_geometry();
```

## File Structure

### Header File Template (.hpp)
```cpp
#pragma once

// System includes
#include <memory>
#include <string>

// Qt includes (if needed)
#include <QString>

// Forward declarations
class UserManager;

namespace core {

/**
 * @brief Brief description of the class
 * 
 * Detailed description of what this class does
 * and how to use it.
 */
class ClassName
{
public:
    // Public types
    enum class State { Active, Inactive };
    
    // Constructors/Destructor
    explicit ClassName(const std::string& name);
    ~ClassName() = default;
    
    // Copy/Move (rule of 5)
    ClassName(const ClassName&) = delete;
    ClassName& operator=(const ClassName&) = delete;
    ClassName(ClassName&&) = default;
    ClassName& operator=(ClassName&&) = default;
    
    // Public interface
    void do_something();
    [[nodiscard]] std::string get_name() const;
    
private:
    // Private members
    std::string name_;
    State state_;
    std::unique_ptr<UserManager> manager_;
};

} // namespace core
```

### Qt Header File Template (.h)
```cpp
#pragma once

#include <QMainWindow>
#include <QString>

QT_BEGIN_NAMESPACE
class QLabel;
class QPushButton;
class QVBoxLayout;
QT_END_NAMESPACE

/**
 * @brief Main application window
 */
class MainWindow : public QMainWindow
{
    Q_OBJECT
    
public:
    explicit MainWindow(QWidget* parent = nullptr);
    ~MainWindow() override = default;

signals:
    void userActionTriggered(const QString& action);
    
private slots:
    void onButtonClicked();
    void handleStatusUpdate();
    
private:
    void setupUi();
    void connectSignals();
    
    // UI components
    QWidget* m_centralWidget;
    QVBoxLayout* m_layout;
    QPushButton* m_actionButton;
    QLabel* m_statusLabel;
    
    // Data members
    QString m_currentStatus;
};
```

## Documentation Standards

### Class Documentation
```cpp
/**
 * @brief Brief one-line description
 * 
 * Detailed description of the class purpose,
 * usage examples, and important notes.
 * 
 * @example
 * UserManager manager("config.json");
 * auto user = manager.create_user("john_doe");
 */
```

### Function Documentation
```cpp
/**
 * @brief Creates a new user with validation
 * 
 * @param username The desired username (3-50 characters)
 * @param email Valid email address
 * @return User object if successful, nullptr otherwise
 * 
 * @throws std::invalid_argument If username or email is invalid
 * @throws DatabaseException If database operation fails
 * 
 * @since Version 2.0
 */
[[nodiscard]] std::unique_ptr<User> create_user(
    const std::string& username,
    const std::string& email
);
```

## Tooling Configuration

### .clang-format
```yaml
---
Language: Cpp
Standard: c++20
IndentWidth: 4
UseTab: Never
ColumnLimit: 100
BreakBeforeBraces: Allman
AllowShortFunctionsOnASingleLine: Empty
AllowShortIfStatementsOnASingleLine: false
AlwaysBreakTemplateDeclarations: true
PointerAlignment: Left
ReferenceAlignment: Left
```

### .clang-tidy
```yaml
---
Checks: >
  *,
  -readability-magic-numbers,
  -cppcoreguidelines-avoid-magic-numbers,
  -fuchsia-default-arguments-calls,
  modernize-*,
  qt-*

CheckOptions:
  - key: readability-identifier-naming.ClassCase
    value: CamelCase
  - key: readability-identifier-naming.FunctionCase
    value: lower_case
  - key: readability-identifier-naming.VariableCase
    value: lower_case
```

### CMakeLists.txt Template
```cmake
cmake_minimum_required(VERSION 3.20)
project(MyProject VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt6 REQUIRED COMPONENTS Core Widgets)

qt6_standard_project_setup()

# Enable Qt MOC, UIC, RCC
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

# Compiler warnings
if(MSVC)
    add_compile_options(/W4)
else()
    add_compile_options(-Wall -Wextra -Wpedantic)
endif()

# Source files
set(SOURCES
    src/main.cpp
    src/ui/MainWindow.cpp
    src/core/user_manager.cpp
)

set(HEADERS
    src/ui/MainWindow.h
    src/core/user_manager.hpp
)

add_executable(${PROJECT_NAME} ${SOURCES} ${HEADERS})

target_link_libraries(${PROJECT_NAME} Qt6::Core Qt6::Widgets)
```

## Examples

### Mixed Qt/C++ Class Example
```cpp
// adapter/user_interface_adapter.hpp
#pragma once

#include <QObject>
#include <QString>
#include <memory>

#include "core/user_manager.hpp"

namespace adapter {

/**
 * @brief Bridges Qt UI and C++ business logic
 */
class UserInterfaceAdapter : public QObject
{
    Q_OBJECT
    
public:
    explicit UserInterfaceAdapter(QObject* parent = nullptr);
    
    // Qt-style interface for UI
    Q_INVOKABLE bool createUser(const QString& username, const QString& email);
    Q_INVOKABLE QString getUserInfo(const QString& username);
    
signals:
    void userCreated(const QString& username);
    void errorOccurred(const QString& message);
    
private:
    // C++ core component
    std::unique_ptr<core::UserManager> m_userManager;
    
    // Conversion helpers
    core::User qtToCoreUser(const QString& username, const QString& email);
    QString coreToQtString(const std::string& str);
};

} // namespace adapter
```

### Modern C++20 Usage
```cpp
// core/data_processor.hpp
#pragma once

#include <concepts>
#include <ranges>
#include <span>
#include <string_view>
#include <vector>

namespace core {

template<typename T>
concept Serializable = requires(T t) {
    { t.serialize() } -> std::convertible_to<std::string>;
    { T::deserialize(std::string_view{}) } -> std::same_as<T>;
};

class DataProcessor
{
public:
    template<Serializable T>
    void process_batch(std::span<const T> items) {
        auto serialized = items 
            | std::views::transform([](const T& item) {
                return item.serialize();
            });
        
        // Process serialized data...
    }
    
    // Using structured bindings
    [[nodiscard]] auto validate_data(std::string_view data) 
        -> std::pair<bool, std::string>;
    
private:
    std::vector<std::string> processed_items_;
};

} // namespace core
```

## Code Review Checklist

### General
- [ ] Consistent naming convention within each layer
- [ ] Proper include order and organization
- [ ] No magic numbers or hardcoded strings
- [ ] Appropriate use of const/constexpr
- [ ] RAII principles followed

### Qt Specific
- [ ] Proper parent-child relationships for QObjects
- [ ] Signals/slots connected correctly
- [ ] Qt containers used appropriately
- [ ] MOC-required macros present (Q_OBJECT, etc.)

### Modern C++
- [ ] Smart pointers used instead of raw pointers
- [ ] Range-based for loops where applicable
- [ ] auto used appropriately
- [ ] Move semantics utilized
- [ ] Concepts used for template constraints

### Performance
- [ ] Pass by const reference for large objects
- [ ] Move semantics for temporary objects
- [ ] Reserve capacity for containers when size is known
- [ ] Avoid unnecessary copies

---

## Version History

- **v1.0** - Initial style guide
- **v1.1** - Added C++20 features section
- **v1.2** - Enhanced Qt integration guidelines

---

*This document is living and should be updated as the project evolves and new patterns emerge.*
