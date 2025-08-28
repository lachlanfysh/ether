#include "ErrorHandler.h"
#include "ErrorReporter.h"
#include <cstring>
#include <algorithm>

// Platform-specific includes for timing
#ifdef STM32H7XX
#include "stm32h7xx_hal.h"
#else
#include <chrono>
#endif

namespace EtherSynth {

ErrorHandler& ErrorHandler::getInstance() {
    static ErrorHandler instance;
    return instance;
}

void ErrorHandler::reportError(const ErrorContext& error) {
    // Update last error info
    lastError_ = error.code;
    lastSeverity_ = error.severity;
    
    // Update error statistics
    if (static_cast<int>(error.severity) < 5) {
        errorCounts_[static_cast<int>(error.severity)]++;
    }
    
    // Get current timestamp
#ifdef STM32H7XX
    uint32_t currentTime = HAL_GetTick();
#else
    auto now = std::chrono::steady_clock::now();
    uint32_t currentTime = std::chrono::duration_cast<std::chrono::milliseconds>(
        now.time_since_epoch()).count();
#endif
    
    // Update error rate tracking
    if (currentTime - lastErrorTime_ < 1000) { // Within 1 second
        errorRateWindow_++;
    } else {
        errorRateWindow_ = 1;
        lastErrorTime_ = currentTime;
    }
    
    // Check if we should log this error
    if (loggingEnabled_ && error.severity >= logLevel_) {
        // In real implementation, this would write to log file or debug output
        // For now, we just store the information
    }
    
    // Call error callback if registered
    if (errorCallback_) {
        errorCallback_(error);
    }
    
    // Report to error reporting system
    ErrorReporter::getInstance().reportError(error);
    
    // For critical/fatal errors, attempt recovery
    if (error.severity >= ErrorSeverity::CRITICAL) {
        attemptRecovery(error.code);
    }
}

void ErrorHandler::reportError(ErrorCode code, ErrorSeverity severity, const char* message) {
    ErrorContext context(code, severity, "unknown", "unknown", 0, message);
    reportError(context);
}

void ErrorHandler::setErrorCallback(ErrorCallback callback) {
    errorCallback_ = callback;
}

void ErrorHandler::setRecoveryCallback(ErrorRecoveryCallback callback) {
    recoveryCallback_ = callback;
}

const char* ErrorHandler::getErrorMessage(ErrorCode code) const {
    switch (code) {
        case ErrorCode::SUCCESS: return "Success";
        
        // General system errors
        case ErrorCode::UNKNOWN_ERROR: return "Unknown error";
        case ErrorCode::OUT_OF_MEMORY: return "Out of memory";
        case ErrorCode::INVALID_PARAMETER: return "Invalid parameter";
        case ErrorCode::NOT_INITIALIZED: return "Not initialized";
        case ErrorCode::ALREADY_INITIALIZED: return "Already initialized";
        case ErrorCode::RESOURCE_UNAVAILABLE: return "Resource unavailable";
        case ErrorCode::TIMEOUT: return "Operation timed out";
        case ErrorCode::PERMISSION_DENIED: return "Permission denied";
        
        // Audio system errors
        case ErrorCode::AUDIO_INIT_FAILED: return "Audio initialization failed";
        case ErrorCode::AUDIO_DEVICE_ERROR: return "Audio device error";
        case ErrorCode::AUDIO_BUFFER_UNDERRUN: return "Audio buffer underrun";
        case ErrorCode::AUDIO_BUFFER_OVERRUN: return "Audio buffer overrun";
        case ErrorCode::AUDIO_SAMPLE_RATE_UNSUPPORTED: return "Unsupported sample rate";
        case ErrorCode::AUDIO_CHANNEL_COUNT_UNSUPPORTED: return "Unsupported channel count";
        case ErrorCode::AUDIO_LATENCY_TOO_HIGH: return "Audio latency too high";
        case ErrorCode::AUDIO_ENGINE_OVERLOAD: return "Audio engine overload";
        
        // Engine errors
        case ErrorCode::ENGINE_INIT_FAILED: return "Engine initialization failed";
        case ErrorCode::ENGINE_INVALID_TYPE: return "Invalid engine type";
        case ErrorCode::ENGINE_VOICE_ALLOCATION_FAILED: return "Voice allocation failed";
        case ErrorCode::ENGINE_PARAMETER_OUT_OF_RANGE: return "Parameter out of range";
        case ErrorCode::ENGINE_PRESET_LOAD_FAILED: return "Preset load failed";
        case ErrorCode::ENGINE_CPU_OVERLOAD: return "Engine CPU overload";
        case ErrorCode::ENGINE_MEMORY_ALLOCATION_FAILED: return "Engine memory allocation failed";
        case ErrorCode::ENGINE_WAVETABLE_LOAD_FAILED: return "Wavetable load failed";
        
        // Hardware interface errors
        case ErrorCode::HARDWARE_INIT_FAILED: return "Hardware initialization failed";
        case ErrorCode::HARDWARE_NOT_FOUND: return "Hardware not found";
        case ErrorCode::HARDWARE_COMMUNICATION_ERROR: return "Hardware communication error";
        case ErrorCode::HARDWARE_FIRMWARE_VERSION_MISMATCH: return "Firmware version mismatch";
        case ErrorCode::SMART_KNOB_INIT_FAILED: return "Smart knob initialization failed";
        case ErrorCode::TOUCH_SCREEN_INIT_FAILED: return "Touch screen initialization failed";
        case ErrorCode::MIDI_INIT_FAILED: return "MIDI initialization failed";
        case ErrorCode::ADC_INIT_FAILED: return "ADC initialization failed";
        case ErrorCode::DAC_INIT_FAILED: return "DAC initialization failed";
        
        // UI system errors
        case ErrorCode::UI_INIT_FAILED: return "UI initialization failed";
        case ErrorCode::UI_GRAPHICS_ERROR: return "Graphics error";
        case ErrorCode::UI_FONT_LOAD_FAILED: return "Font load failed";
        case ErrorCode::UI_TOUCH_CALIBRATION_FAILED: return "Touch calibration failed";
        case ErrorCode::UI_SCREEN_UPDATE_FAILED: return "Screen update failed";
        case ErrorCode::UI_THEME_LOAD_FAILED: return "Theme load failed";
        
        // File system errors
        case ErrorCode::FILE_SYSTEM_ERROR: return "File system error";
        case ErrorCode::FILE_NOT_FOUND: return "File not found";
        case ErrorCode::FILE_READ_ERROR: return "File read error";
        case ErrorCode::FILE_WRITE_ERROR: return "File write error";
        case ErrorCode::FILE_PERMISSION_ERROR: return "File permission error";
        case ErrorCode::DISK_FULL: return "Disk full";
        case ErrorCode::INVALID_FILE_FORMAT: return "Invalid file format";
        case ErrorCode::FILE_CORRUPTED: return "File corrupted";
        
        // Sample/preset errors
        case ErrorCode::SAMPLE_LOAD_FAILED: return "Sample load failed";
        case ErrorCode::SAMPLE_FORMAT_UNSUPPORTED: return "Unsupported sample format";
        case ErrorCode::SAMPLE_SIZE_TOO_LARGE: return "Sample size too large";
        case ErrorCode::PRESET_SAVE_FAILED: return "Preset save failed";
        case ErrorCode::PRESET_LOAD_FAILED: return "Preset load failed";
        case ErrorCode::PRESET_FORMAT_INVALID: return "Invalid preset format";
        case ErrorCode::SAMPLE_LIBRARY_INIT_FAILED: return "Sample library initialization failed";
        
        // Modulation system errors
        case ErrorCode::MODULATION_INIT_FAILED: return "Modulation system initialization failed";
        case ErrorCode::MODULATION_MATRIX_OVERFLOW: return "Modulation matrix overflow";
        case ErrorCode::MODULATION_SOURCE_INVALID: return "Invalid modulation source";
        case ErrorCode::MODULATION_DESTINATION_INVALID: return "Invalid modulation destination";
        case ErrorCode::LFO_INIT_FAILED: return "LFO initialization failed";
        case ErrorCode::ENVELOPE_INIT_FAILED: return "Envelope initialization failed";
        
        // Network/sync errors
        case ErrorCode::NETWORK_INIT_FAILED: return "Network initialization failed";
        case ErrorCode::SYNC_LOST: return "Sync lost";
        case ErrorCode::CLOCK_SYNC_ERROR: return "Clock sync error";
        case ErrorCode::MIDI_SYNC_ERROR: return "MIDI sync error";
        
        // Performance/resource errors
        case ErrorCode::CPU_OVERLOAD: return "CPU overload";
        case ErrorCode::MEMORY_FRAGMENTATION: return "Memory fragmentation";
        case ErrorCode::REAL_TIME_VIOLATION: return "Real-time violation";
        case ErrorCode::THREAD_PRIORITY_ERROR: return "Thread priority error";
        case ErrorCode::CACHE_MISS_RATE_HIGH: return "High cache miss rate";
        
        default: return "Unknown error code";
    }
}

uint32_t ErrorHandler::getErrorCount(ErrorSeverity severity) const {
    int index = static_cast<int>(severity);
    return (index < 5) ? errorCounts_[index] : 0;
}

void ErrorHandler::setLogLevel(ErrorSeverity minSeverity) {
    logLevel_ = minSeverity;
}

void ErrorHandler::enableErrorLogging(bool enabled) {
    loggingEnabled_ = enabled;
}

void ErrorHandler::clearErrorHistory() {
    lastError_ = ErrorCode::SUCCESS;
    lastSeverity_ = ErrorSeverity::INFO;
    for (int i = 0; i < 5; i++) {
        errorCounts_[i] = 0;
    }
    errorRateWindow_ = 0;
}

bool ErrorHandler::isSystemHealthy() const {
    // System is considered unhealthy if:
    // - Too many critical errors
    // - High error rate
    // - Fatal error occurred
    
    uint32_t criticalErrors = errorCounts_[static_cast<int>(ErrorSeverity::CRITICAL)];
    uint32_t fatalErrors = errorCounts_[static_cast<int>(ErrorSeverity::FATAL)];
    
    if (fatalErrors > 0) return false;
    if (criticalErrors > 5) return false;  // More than 5 critical errors
    if (errorRateWindow_ > 10) return false;  // More than 10 errors per second
    
    return true;
}

float ErrorHandler::getErrorRate() const {
    return static_cast<float>(errorRateWindow_);
}

bool ErrorHandler::attemptRecovery(ErrorCode code) {
    if (recoveryCallback_) {
        ErrorContext context(code, ErrorSeverity::CRITICAL, "recovery", "recovery", 0);
        return recoveryCallback_(context);
    }
    return false;
}

void ErrorHandler::registerRecoveryStrategy(ErrorCode code, ErrorRecoveryCallback strategy) {
    // In a full implementation, this would store per-code recovery strategies
    // For now, we just store a single global callback
    recoveryCallback_ = strategy;
}

} // namespace EtherSynth