#pragma once

#include "logger.h"

/**
 * @brief Compatibility wrapper for legacy Logger interface
 */
class LoggerCompat {
public:
    enum LogLevel {
        Debug = 0,
        Info = 1,
        Warning = 2,
        Error = 3,
        Critical = 4
    };

    static LoggerCompat& instance() {
        static LoggerCompat s_instance;
        return s_instance;
    }

    void log(LogLevel level, const QString& category, const QString& message) {
        // Simple console output for now to make build work
        qDebug() << "[" << levelToString(level) << "]" << category << ":" << message;
    }

private:
    QString levelToString(LogLevel level) {
        switch(level) {
            case Debug: return "DEBUG";
            case Info: return "INFO";
            case Warning: return "WARNING";
            case Error: return "ERROR";
            case Critical: return "CRITICAL";
            default: return "UNKNOWN";
        }
    }
};

// Alias for compatibility
using Logger = LoggerCompat;