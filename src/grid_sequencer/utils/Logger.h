#pragma once
#include <iostream>
#include <fstream>
#include <string>
#include <chrono>
#include <iomanip>
#include <sstream>

namespace GridSequencer {
namespace Utils {

enum class LogLevel {
    DEBUG = 0,
    INFO = 1,
    WARNING = 2,
    ERROR = 3
};

class Logger {
public:
    static Logger& getInstance() {
        static Logger instance;
        return instance;
    }

    void setLogLevel(LogLevel level) {
        logLevel_ = level;
    }

    void setLogFile(const std::string& filename) {
        if (logFile_.is_open()) {
            logFile_.close();
        }
        logFile_.open(filename, std::ios::app);
    }

    void log(LogLevel level, const std::string& message) {
        if (level < logLevel_) return;

        auto now = std::chrono::system_clock::now();
        auto time_t = std::chrono::system_clock::to_time_t(now);

        std::stringstream ss;
        ss << std::put_time(std::localtime(&time_t), "%Y-%m-%d %H:%M:%S");
        ss << " [" << levelToString(level) << "] " << message;

        std::string logEntry = ss.str();

        // Always log to console
        std::cout << logEntry << std::endl;

        // Also log to file if available
        if (logFile_.is_open()) {
            logFile_ << logEntry << std::endl;
            logFile_.flush();
        }
    }

    void debug(const std::string& message) { log(LogLevel::DEBUG, message); }
    void info(const std::string& message) { log(LogLevel::INFO, message); }
    void warning(const std::string& message) { log(LogLevel::WARNING, message); }
    void error(const std::string& message) { log(LogLevel::ERROR, message); }

private:
    Logger() : logLevel_(LogLevel::INFO) {}
    ~Logger() {
        if (logFile_.is_open()) {
            logFile_.close();
        }
    }

    std::string levelToString(LogLevel level) {
        switch (level) {
            case LogLevel::DEBUG: return "DEBUG";
            case LogLevel::INFO: return "INFO";
            case LogLevel::WARNING: return "WARN";
            case LogLevel::ERROR: return "ERROR";
            default: return "UNKNOWN";
        }
    }

    LogLevel logLevel_;
    std::ofstream logFile_;
};

// Convenience macros
#define LOG_DEBUG(msg) GridSequencer::Utils::Logger::getInstance().debug(msg)
#define LOG_INFO(msg) GridSequencer::Utils::Logger::getInstance().info(msg)
#define LOG_WARNING(msg) GridSequencer::Utils::Logger::getInstance().warning(msg)
#define LOG_ERROR(msg) GridSequencer::Utils::Logger::getInstance().error(msg)

} // namespace Utils
} // namespace GridSequencer