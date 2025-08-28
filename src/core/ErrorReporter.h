#pragma once
#include "ErrorHandler.h"
#include <vector>
#include <deque>
#include <fstream>
#include <mutex>
#include <string>
#include <memory>

namespace EtherSynth {

/**
 * Comprehensive error reporting and logging system for EtherSynth V1.0
 * 
 * Features:
 * - Real-time error logging to file and debug output
 * - Error history with ring buffer for memory efficiency
 * - Performance metrics and system health monitoring
 * - Error correlation and pattern detection
 * - Remote error reporting (future: telemetry)
 * - Debug console integration
 * - Error visualization for development
 */

// Forward declarations
class DebugConsole;
class ErrorVisualizer;

// Error log entry structure
struct ErrorLogEntry {
    uint32_t timestamp;
    ErrorCode errorCode;
    ErrorSeverity severity;
    std::string function;
    std::string file;
    int line;
    std::string message;
    std::string subsystem;      // "Audio", "Engine", "UI", etc.
    float systemLoadAtError;    // CPU/memory load when error occurred
    uint32_t errorCount;        // How many times this error has occurred
    
    ErrorLogEntry() : timestamp(0), errorCode(ErrorCode::SUCCESS), 
                     severity(ErrorSeverity::INFO), line(0), 
                     systemLoadAtError(0.0f), errorCount(1) {}
};

// System health metrics
struct SystemHealthMetrics {
    float cpuUsage;             // 0-100%
    float memoryUsage;          // 0-100%
    float audioDropoutRate;     // dropouts per second
    float errorRate;            // errors per second
    uint32_t totalErrors;       // total error count since boot
    uint32_t criticalErrors;    // critical error count
    uint32_t uptime;           // system uptime in milliseconds
    bool isHealthy;            // overall health status
    
    SystemHealthMetrics() : cpuUsage(0.0f), memoryUsage(0.0f), 
                           audioDropoutRate(0.0f), errorRate(0.0f),
                           totalErrors(0), criticalErrors(0), 
                           uptime(0), isHealthy(true) {}
};

// Error reporting configuration
struct ErrorReportingConfig {
    bool enableFileLogging;     // Log to file
    bool enableConsoleOutput;   // Output to debug console
    bool enableVisualization;   // Show error visualization
    bool enableTelemetry;       // Remote error reporting
    ErrorSeverity minLogLevel;  // Minimum severity to log
    size_t maxLogEntries;       // Maximum entries in memory
    std::string logFilePath;    // Path to log file
    bool compressOldLogs;       // Compress old log files
    uint32_t logRotationSize;   // Size threshold for log rotation (MB)
    
    ErrorReportingConfig() : enableFileLogging(true), enableConsoleOutput(true),
                            enableVisualization(false), enableTelemetry(false),
                            minLogLevel(ErrorSeverity::WARNING), maxLogEntries(1000),
                            logFilePath("ether_errors.log"), compressOldLogs(true),
                            logRotationSize(10) {}
};

// Error pattern detection
struct ErrorPattern {
    ErrorCode errorCode;
    uint32_t occurrences;       // How many times this error occurred
    uint32_t timeWindow;        // Time window for pattern detection (ms)
    float frequency;            // Errors per second in the time window
    bool isRecurring;          // True if error occurs regularly
    std::string description;    // Pattern description
    
    ErrorPattern() : errorCode(ErrorCode::SUCCESS), occurrences(0), 
                    timeWindow(0), frequency(0.0f), isRecurring(false) {}
};

/**
 * Main error reporting class
 */
class ErrorReporter {
public:
    static ErrorReporter& getInstance();
    ~ErrorReporter();
    
    // Configuration
    void initialize(const ErrorReportingConfig& config);
    void setConfig(const ErrorReportingConfig& config);
    const ErrorReportingConfig& getConfig() const { return config_; }
    
    // Error reporting (called by ErrorHandler)
    void reportError(const ErrorContext& error);
    void reportSystemHealth(const SystemHealthMetrics& metrics);
    
    // Log access
    std::vector<ErrorLogEntry> getRecentErrors(int count = 50) const;
    std::vector<ErrorLogEntry> getErrorsByType(ErrorCode code) const;
    std::vector<ErrorLogEntry> getErrorsBySeverity(ErrorSeverity severity) const;
    ErrorLogEntry getLastError() const;
    
    // Statistics and analysis
    SystemHealthMetrics getCurrentHealthMetrics() const;
    std::vector<ErrorPattern> detectErrorPatterns() const;
    float getErrorRateForCode(ErrorCode code) const;
    std::string generateSystemReport() const;
    std::string generateErrorSummary() const;
    
    // Pattern detection
    bool isErrorRecurring(ErrorCode code, uint32_t timeWindowMs = 60000) const;
    void updateErrorPatterns();
    void clearErrorPatterns();
    
    // File logging
    bool openLogFile();
    void closeLogFile();
    void rotateLogFile();
    void flushLogFile();
    
    // Debug integration
    void setDebugConsole(std::shared_ptr<DebugConsole> console);
    void setErrorVisualizer(std::shared_ptr<ErrorVisualizer> visualizer);
    
    // Telemetry (future)
    void enableTelemetry(bool enabled);
    bool uploadErrorReport();
    
    // Cleanup and maintenance
    void clearOldEntries();
    void exportErrorLog(const std::string& exportPath) const;
    size_t getLogSizeBytes() const;
    
private:
    ErrorReporter() = default;
    
    // Configuration
    ErrorReportingConfig config_;
    
    // Error storage (ring buffer for efficiency)
    std::deque<ErrorLogEntry> errorLog_;
    mutable std::mutex logMutex_;
    
    // Pattern detection
    std::vector<ErrorPattern> errorPatterns_;
    std::mutex patternMutex_;
    
    // File logging
    std::unique_ptr<std::ofstream> logFile_;
    std::string currentLogPath_;
    size_t currentLogSize_;
    
    // System health tracking
    SystemHealthMetrics currentHealth_;
    std::deque<SystemHealthMetrics> healthHistory_;
    
    // External integrations
    std::shared_ptr<DebugConsole> debugConsole_;
    std::shared_ptr<ErrorVisualizer> errorVisualizer_;
    
    // Internal helpers
    void writeToFile(const ErrorLogEntry& entry);
    void writeToConsole(const ErrorLogEntry& entry);
    void updateVisualization(const ErrorLogEntry& entry);
    void checkLogRotation();
    void compressOldLog(const std::string& logPath);
    std::string formatLogEntry(const ErrorLogEntry& entry) const;
    std::string getTimestamp(uint32_t timestamp) const;
    std::string getSubsystemFromError(ErrorCode code) const;
    float getCurrentCPUUsage() const;
    float getCurrentMemoryUsage() const;
    uint32_t getCurrentTimestamp() const;
    void trimLogToMaxSize();
    void updateHealthStatus();
    
    // Pattern analysis
    void addErrorToPattern(ErrorCode code, uint32_t timestamp);
    ErrorPattern* findPattern(ErrorCode code);
    void updatePatternFrequency(ErrorPattern& pattern, uint32_t currentTime);
};

/**
 * Debug console interface for real-time error monitoring
 */
class DebugConsole {
public:
    virtual ~DebugConsole() = default;
    
    virtual void printError(const ErrorLogEntry& error) = 0;
    virtual void printSystemHealth(const SystemHealthMetrics& health) = 0;
    virtual void printErrorSummary(const std::string& summary) = 0;
    virtual void clearConsole() = 0;
    virtual void setLogLevel(ErrorSeverity minLevel) = 0;
};

/**
 * Error visualization interface for development debugging
 */
class ErrorVisualizer {
public:
    virtual ~ErrorVisualizer() = default;
    
    virtual void showError(const ErrorLogEntry& error) = 0;
    virtual void updateHealthDisplay(const SystemHealthMetrics& health) = 0;
    virtual void showErrorPatterns(const std::vector<ErrorPattern>& patterns) = 0;
    virtual void highlightErrorLocation(const std::string& file, int line) = 0;
    virtual void showSystemReport(const std::string& report) = 0;
};

/**
 * Simple console implementation for basic debugging
 */
class SimpleDebugConsole : public DebugConsole {
public:
    SimpleDebugConsole(ErrorSeverity minLevel = ErrorSeverity::WARNING);
    
    void printError(const ErrorLogEntry& error) override;
    void printSystemHealth(const SystemHealthMetrics& health) override;
    void printErrorSummary(const std::string& summary) override;
    void clearConsole() override;
    void setLogLevel(ErrorSeverity minLevel) override { minLevel_ = minLevel; }
    
private:
    ErrorSeverity minLevel_;
    const char* getSeverityString(ErrorSeverity severity) const;
    const char* getSeverityColor(ErrorSeverity severity) const;
};

// Convenience macros for error reporting integration
#define ETHER_REPORT_ERROR(context) \
    EtherSynth::ErrorReporter::getInstance().reportError(context)

#define ETHER_LOG_SYSTEM_HEALTH(metrics) \
    EtherSynth::ErrorReporter::getInstance().reportSystemHealth(metrics)

#define ETHER_CHECK_RECURRING_ERROR(code) \
    EtherSynth::ErrorReporter::getInstance().isErrorRecurring(code)

} // namespace EtherSynth