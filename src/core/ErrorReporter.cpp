#include "ErrorReporter.h"
#include <iostream>
#include <sstream>
#include <algorithm>
#include <ctime>

// Platform-specific includes
#ifdef STM32H7XX
#include "stm32h7xx_hal.h"
#else
#include <chrono>
#endif

namespace EtherSynth {

ErrorReporter& ErrorReporter::getInstance() {
    static ErrorReporter instance;
    return instance;
}

ErrorReporter::~ErrorReporter() {
    closeLogFile();
}

void ErrorReporter::initialize(const ErrorReportingConfig& config) {
    config_ = config;
    
    if (config_.enableFileLogging) {
        openLogFile();
    }
    
    // Initialize debug console if needed
    if (config_.enableConsoleOutput && !debugConsole_) {
        debugConsole_ = std::make_shared<SimpleDebugConsole>(config_.minLogLevel);
    }
    
    // Clear old entries to start fresh
    clearOldEntries();
}

void ErrorReporter::reportError(const ErrorContext& error) {
    std::lock_guard<std::mutex> lock(logMutex_);
    
    // Create log entry
    ErrorLogEntry entry;
    entry.timestamp = getCurrentTimestamp();
    entry.errorCode = error.code;
    entry.severity = error.severity;
    entry.function = error.function ? error.function : "unknown";
    entry.file = error.file ? error.file : "unknown";
    entry.line = error.line;
    entry.message = error.message ? error.message : "";
    entry.subsystem = getSubsystemFromError(error.code);
    entry.systemLoadAtError = getCurrentCPUUsage();
    
    // Check if this is a duplicate error
    if (!errorLog_.empty()) {
        const auto& lastEntry = errorLog_.back();
        if (lastEntry.errorCode == error.code && 
            (entry.timestamp - lastEntry.timestamp) < 1000) { // Within 1 second
            // Update count of existing entry instead of creating new one
            const_cast<ErrorLogEntry&>(lastEntry).errorCount++;
            return;
        }
    }
    
    // Add to log
    errorLog_.push_back(entry);
    
    // Maintain ring buffer size
    if (errorLog_.size() > config_.maxLogEntries) {
        errorLog_.pop_front();
    }
    
    // Update patterns
    addErrorToPattern(error.code, entry.timestamp);
    
    // Write to various outputs
    if (config_.enableFileLogging && logFile_) {
        writeToFile(entry);
    }
    
    if (config_.enableConsoleOutput && debugConsole_) {
        writeToConsole(entry);
    }
    
    if (config_.enableVisualization && errorVisualizer_) {
        updateVisualization(entry);
    }
    
    // Update system health
    updateHealthStatus();
}

void ErrorReporter::reportSystemHealth(const SystemHealthMetrics& metrics) {
    std::lock_guard<std::mutex> lock(logMutex_);
    
    currentHealth_ = metrics;
    healthHistory_.push_back(metrics);
    
    // Keep only recent health history
    if (healthHistory_.size() > 100) {
        healthHistory_.pop_front();
    }
    
    // Report to console if health is degraded
    if (!metrics.isHealthy && debugConsole_) {
        debugConsole_->printSystemHealth(metrics);
    }
}

std::vector<ErrorLogEntry> ErrorReporter::getRecentErrors(int count) const {
    std::lock_guard<std::mutex> lock(logMutex_);
    
    std::vector<ErrorLogEntry> result;
    int startIdx = std::max(0, static_cast<int>(errorLog_.size()) - count);
    
    for (int i = startIdx; i < static_cast<int>(errorLog_.size()); ++i) {
        result.push_back(errorLog_[i]);
    }
    
    return result;
}

std::vector<ErrorLogEntry> ErrorReporter::getErrorsByType(ErrorCode code) const {
    std::lock_guard<std::mutex> lock(logMutex_);
    
    std::vector<ErrorLogEntry> result;
    for (const auto& entry : errorLog_) {
        if (entry.errorCode == code) {
            result.push_back(entry);
        }
    }
    
    return result;
}

ErrorLogEntry ErrorReporter::getLastError() const {
    std::lock_guard<std::mutex> lock(logMutex_);
    
    if (errorLog_.empty()) {
        return ErrorLogEntry();
    }
    
    return errorLog_.back();
}

SystemHealthMetrics ErrorReporter::getCurrentHealthMetrics() const {
    return currentHealth_;
}

std::string ErrorReporter::generateSystemReport() const {
    std::lock_guard<std::mutex> lock(logMutex_);
    
    std::ostringstream report;
    
    report << "EtherSynth V1.0 System Report\n";
    report << "============================\n\n";
    
    // System health
    report << "System Health:\n";
    report << "  CPU Usage: " << currentHealth_.cpuUsage << "%\n";
    report << "  Memory Usage: " << currentHealth_.memoryUsage << "%\n";
    report << "  Audio Dropouts: " << currentHealth_.audioDropoutRate << "/sec\n";
    report << "  Error Rate: " << currentHealth_.errorRate << "/sec\n";
    report << "  Uptime: " << currentHealth_.uptime / 1000 << " seconds\n";
    report << "  Overall Health: " << (currentHealth_.isHealthy ? "HEALTHY" : "DEGRADED") << "\n\n";
    
    // Error statistics
    uint32_t totalErrors = 0;
    uint32_t warningCount = 0;
    uint32_t errorCount = 0;
    uint32_t criticalCount = 0;
    
    for (const auto& entry : errorLog_) {
        totalErrors += entry.errorCount;
        switch (entry.severity) {
            case ErrorSeverity::WARNING: warningCount++; break;
            case ErrorSeverity::ERROR: errorCount++; break;
            case ErrorSeverity::CRITICAL: criticalCount++; break;
            case ErrorSeverity::FATAL: criticalCount++; break;
            default: break;
        }
    }
    
    report << "Error Statistics:\n";
    report << "  Total Errors: " << totalErrors << "\n";
    report << "  Warnings: " << warningCount << "\n";
    report << "  Errors: " << errorCount << "\n";
    report << "  Critical/Fatal: " << criticalCount << "\n";
    report << "  Log Entries: " << errorLog_.size() << "\n\n";
    
    // Recent errors
    report << "Recent Errors (last 10):\n";
    int count = 0;
    for (auto it = errorLog_.rbegin(); it != errorLog_.rend() && count < 10; ++it, ++count) {
        report << "  [" << getTimestamp(it->timestamp) << "] " 
               << it->subsystem << " - " 
               << ErrorHandler::getInstance().getErrorMessage(it->errorCode) << "\n";
    }
    
    return report.str();
}

bool ErrorReporter::isErrorRecurring(ErrorCode code, uint32_t timeWindowMs) const {
    std::lock_guard<std::mutex> lock(const_cast<std::mutex&>(patternMutex_));
    
    const ErrorPattern* pattern = const_cast<ErrorReporter*>(this)->findPattern(code);
    if (!pattern) return false;
    
    return pattern->isRecurring && (pattern->frequency > 0.1f); // More than 0.1 errors/sec
}

bool ErrorReporter::openLogFile() {
    if (!config_.enableFileLogging) return false;
    
    currentLogPath_ = config_.logFilePath;
    logFile_ = std::make_unique<std::ofstream>(currentLogPath_, std::ios::app);
    
    if (logFile_->is_open()) {
        *logFile_ << "\n=== EtherSynth V1.0 Session Started at " 
                  << getTimestamp(getCurrentTimestamp()) << " ===\n";
        logFile_->flush();
        return true;
    }
    
    return false;
}

void ErrorReporter::closeLogFile() {
    if (logFile_ && logFile_->is_open()) {
        *logFile_ << "=== Session Ended ===\n";
        logFile_->close();
    }
    logFile_.reset();
}

void ErrorReporter::writeToFile(const ErrorLogEntry& entry) {
    if (!logFile_ || !logFile_->is_open()) return;
    
    std::string formattedEntry = formatLogEntry(entry);
    *logFile_ << formattedEntry << std::endl;
    logFile_->flush();
    
    currentLogSize_ += formattedEntry.length();
    checkLogRotation();
}

void ErrorReporter::writeToConsole(const ErrorLogEntry& entry) {
    if (debugConsole_ && entry.severity >= config_.minLogLevel) {
        debugConsole_->printError(entry);
    }
}

void ErrorReporter::updateVisualization(const ErrorLogEntry& entry) {
    if (errorVisualizer_) {
        errorVisualizer_->showError(entry);
    }
}

std::string ErrorReporter::formatLogEntry(const ErrorLogEntry& entry) const {
    std::ostringstream formatted;
    
    formatted << "[" << getTimestamp(entry.timestamp) << "] "
              << "[" << static_cast<int>(entry.severity) << "] "
              << "[" << entry.subsystem << "] "
              << ErrorHandler::getInstance().getErrorMessage(entry.errorCode);
    
    if (!entry.message.empty()) {
        formatted << " - " << entry.message;
    }
    
    formatted << " (" << entry.function << " at " << entry.file << ":" << entry.line << ")";
    
    if (entry.errorCount > 1) {
        formatted << " [x" << entry.errorCount << "]";
    }
    
    return formatted.str();
}

std::string ErrorReporter::getTimestamp(uint32_t timestamp) const {
#ifdef STM32H7XX
    // Simple timestamp for embedded systems
    uint32_t seconds = timestamp / 1000;
    uint32_t ms = timestamp % 1000;
    
    char buffer[32];
    snprintf(buffer, sizeof(buffer), "%lu.%03lu", seconds, ms);
    return std::string(buffer);
#else
    auto time_point = std::chrono::system_clock::from_time_t(timestamp / 1000);
    auto time_t = std::chrono::system_clock::to_time_t(time_point);
    
    char buffer[100];
    std::strftime(buffer, sizeof(buffer), "%H:%M:%S", std::localtime(&time_t));
    
    // Add milliseconds
    uint32_t ms = timestamp % 1000;
    std::string result = buffer;
    result += "." + std::to_string(ms);
    
    return result;
#endif
}

std::string ErrorReporter::getSubsystemFromError(ErrorCode code) const {
    uint16_t codeValue = static_cast<uint16_t>(code);
    
    if (codeValue >= 100 && codeValue < 200) return "Audio";
    if (codeValue >= 200 && codeValue < 300) return "Engine";
    if (codeValue >= 300 && codeValue < 400) return "Hardware";
    if (codeValue >= 400 && codeValue < 500) return "UI";
    if (codeValue >= 500 && codeValue < 600) return "FileSystem";
    if (codeValue >= 600 && codeValue < 700) return "Sample";
    if (codeValue >= 700 && codeValue < 800) return "Modulation";
    if (codeValue >= 800 && codeValue < 900) return "Network";
    if (codeValue >= 900) return "Performance";
    
    return "System";
}

uint32_t ErrorReporter::getCurrentTimestamp() const {
#ifdef STM32H7XX
    return HAL_GetTick();
#else
    auto now = std::chrono::steady_clock::now();
    return std::chrono::duration_cast<std::chrono::milliseconds>(
        now.time_since_epoch()).count();
#endif
}

float ErrorReporter::getCurrentCPUUsage() const {
    // Placeholder - would need platform-specific implementation
    return 0.0f;
}

void ErrorReporter::addErrorToPattern(ErrorCode code, uint32_t timestamp) {
    std::lock_guard<std::mutex> lock(patternMutex_);
    
    ErrorPattern* pattern = findPattern(code);
    if (!pattern) {
        errorPatterns_.emplace_back();
        pattern = &errorPatterns_.back();
        pattern->errorCode = code;
        pattern->timeWindow = timestamp;
    }
    
    pattern->occurrences++;
    updatePatternFrequency(*pattern, timestamp);
}

ErrorPattern* ErrorReporter::findPattern(ErrorCode code) {
    for (auto& pattern : errorPatterns_) {
        if (pattern.errorCode == code) {
            return &pattern;
        }
    }
    return nullptr;
}

void ErrorReporter::updatePatternFrequency(ErrorPattern& pattern, uint32_t currentTime) {
    uint32_t timeDiff = currentTime - pattern.timeWindow;
    if (timeDiff > 0) {
        pattern.frequency = static_cast<float>(pattern.occurrences) / (timeDiff / 1000.0f);
        pattern.isRecurring = (pattern.occurrences > 3 && pattern.frequency > 0.1f);
    }
}

void ErrorReporter::updateHealthStatus() {
    // Simple health calculation based on recent errors
    uint32_t recentCriticalErrors = 0;
    uint32_t recentErrors = 0;
    uint32_t currentTime = getCurrentTimestamp();
    
    for (const auto& entry : errorLog_) {
        if ((currentTime - entry.timestamp) < 60000) { // Last minute
            recentErrors++;
            if (entry.severity >= ErrorSeverity::CRITICAL) {
                recentCriticalErrors++;
            }
        }
    }
    
    currentHealth_.totalErrors = errorLog_.size();
    currentHealth_.criticalErrors = recentCriticalErrors;
    currentHealth_.errorRate = static_cast<float>(recentErrors) / 60.0f; // Errors per second
    currentHealth_.uptime = currentTime;
    currentHealth_.isHealthy = (recentCriticalErrors == 0 && recentErrors < 10);
}

void ErrorReporter::checkLogRotation() {
    if (currentLogSize_ > config_.logRotationSize * 1024 * 1024) { // MB to bytes
        rotateLogFile();
    }
}

void ErrorReporter::rotateLogFile() {
    if (!logFile_) return;
    
    logFile_->close();
    
    // Create backup filename with timestamp
    std::string backupPath = currentLogPath_ + "." + std::to_string(getCurrentTimestamp());
    std::rename(currentLogPath_.c_str(), backupPath.c_str());
    
    // Reopen main log file
    logFile_->open(currentLogPath_, std::ios::trunc);
    currentLogSize_ = 0;
    
    // Optionally compress old log
    if (config_.compressOldLogs) {
        compressOldLog(backupPath);
    }
}

void ErrorReporter::compressOldLog(const std::string& logPath) {
    // Placeholder for compression logic
    // In a real implementation, this would use zlib or similar
}

void ErrorReporter::clearOldEntries() {
    std::lock_guard<std::mutex> lock(logMutex_);
    
    if (errorLog_.size() > config_.maxLogEntries) {
        size_t excess = errorLog_.size() - config_.maxLogEntries;
        errorLog_.erase(errorLog_.begin(), errorLog_.begin() + excess);
    }
}

//=============================================================================
// SimpleDebugConsole Implementation
//=============================================================================

SimpleDebugConsole::SimpleDebugConsole(ErrorSeverity minLevel) : minLevel_(minLevel) {
}

void SimpleDebugConsole::printError(const ErrorLogEntry& error) {
    if (error.severity < minLevel_) return;
    
    const char* color = getSeverityColor(error.severity);
    const char* severityStr = getSeverityString(error.severity);
    
    std::cout << color << "[" << severityStr << "] " 
              << error.subsystem << ": " 
              << ErrorHandler::getInstance().getErrorMessage(error.errorCode);
    
    if (!error.message.empty()) {
        std::cout << " - " << error.message;
    }
    
    if (error.errorCount > 1) {
        std::cout << " [x" << error.errorCount << "]";
    }
    
    std::cout << "\033[0m" << std::endl; // Reset color
}

void SimpleDebugConsole::printSystemHealth(const SystemHealthMetrics& health) {
    const char* color = health.isHealthy ? "\033[32m" : "\033[31m"; // Green or Red
    
    std::cout << color << "System Health: " << (health.isHealthy ? "HEALTHY" : "DEGRADED")
              << " (CPU: " << health.cpuUsage << "%, Mem: " << health.memoryUsage << "%, "
              << "Errors: " << health.errorRate << "/sec)\033[0m" << std::endl;
}

void SimpleDebugConsole::printErrorSummary(const std::string& summary) {
    std::cout << "\n" << summary << std::endl;
}

void SimpleDebugConsole::clearConsole() {
    // Platform-specific console clearing
#ifdef _WIN32
    system("cls");
#else
    system("clear");
#endif
}

const char* SimpleDebugConsole::getSeverityString(ErrorSeverity severity) const {
    switch (severity) {
        case ErrorSeverity::INFO: return "INFO";
        case ErrorSeverity::WARNING: return "WARN";
        case ErrorSeverity::ERROR: return "ERROR";
        case ErrorSeverity::CRITICAL: return "CRITICAL";
        case ErrorSeverity::FATAL: return "FATAL";
        default: return "UNKNOWN";
    }
}

const char* SimpleDebugConsole::getSeverityColor(ErrorSeverity severity) const {
    switch (severity) {
        case ErrorSeverity::INFO: return "\033[37m";      // White
        case ErrorSeverity::WARNING: return "\033[33m";   // Yellow
        case ErrorSeverity::ERROR: return "\033[31m";     // Red
        case ErrorSeverity::CRITICAL: return "\033[35m";  // Magenta
        case ErrorSeverity::FATAL: return "\033[41m";     // Red background
        default: return "\033[0m";                         // Reset
    }
}

} // namespace EtherSynth