#pragma once
#include <string>
#include <cstdint>
#include <functional>

namespace EtherSynth {

/**
 * Comprehensive error handling system for EtherSynth V1.0
 * 
 * Provides consistent error reporting, logging, and recovery across all subsystems:
 * - Standardized error codes and severity levels
 * - Error logging with automatic context capture
 * - Error recovery strategies and callbacks
 * - Performance-optimized for real-time audio constraints
 * - Integration with existing bool return patterns
 */

// Error severity levels
enum class ErrorSeverity : uint8_t {
    INFO = 0,       // Informational (system events, state changes)
    WARNING,        // Warnings (non-critical issues, fallback used)
    ERROR,          // Errors (functionality impaired, but recoverable)
    CRITICAL,       // Critical errors (major functionality lost)
    FATAL           // Fatal errors (system must shut down)
};

// Comprehensive error codes for all subsystems
enum class ErrorCode : uint16_t {
    // Success
    SUCCESS = 0,
    
    // General system errors (1-99)
    UNKNOWN_ERROR = 1,
    OUT_OF_MEMORY = 2,
    INVALID_PARAMETER = 3,
    NOT_INITIALIZED = 4,
    ALREADY_INITIALIZED = 5,
    RESOURCE_UNAVAILABLE = 6,
    TIMEOUT = 7,
    PERMISSION_DENIED = 8,
    
    // Audio system errors (100-199)
    AUDIO_INIT_FAILED = 100,
    AUDIO_DEVICE_ERROR = 101,
    AUDIO_BUFFER_UNDERRUN = 102,
    AUDIO_BUFFER_OVERRUN = 103,
    AUDIO_SAMPLE_RATE_UNSUPPORTED = 104,
    AUDIO_CHANNEL_COUNT_UNSUPPORTED = 105,
    AUDIO_LATENCY_TOO_HIGH = 106,
    AUDIO_ENGINE_OVERLOAD = 107,
    
    // Engine errors (200-299)
    ENGINE_INIT_FAILED = 200,
    ENGINE_INVALID_TYPE = 201,
    ENGINE_VOICE_ALLOCATION_FAILED = 202,
    ENGINE_PARAMETER_OUT_OF_RANGE = 203,
    ENGINE_PRESET_LOAD_FAILED = 204,
    ENGINE_CPU_OVERLOAD = 205,
    ENGINE_MEMORY_ALLOCATION_FAILED = 206,
    ENGINE_WAVETABLE_LOAD_FAILED = 207,
    
    // Hardware interface errors (300-399)
    HARDWARE_INIT_FAILED = 300,
    HARDWARE_NOT_FOUND = 301,
    HARDWARE_COMMUNICATION_ERROR = 302,
    HARDWARE_FIRMWARE_VERSION_MISMATCH = 303,
    SMART_KNOB_INIT_FAILED = 304,
    TOUCH_SCREEN_INIT_FAILED = 305,
    MIDI_INIT_FAILED = 306,
    ADC_INIT_FAILED = 307,
    DAC_INIT_FAILED = 308,
    
    // UI system errors (400-499)
    UI_INIT_FAILED = 400,
    UI_GRAPHICS_ERROR = 401,
    UI_FONT_LOAD_FAILED = 402,
    UI_TOUCH_CALIBRATION_FAILED = 403,
    UI_SCREEN_UPDATE_FAILED = 404,
    UI_THEME_LOAD_FAILED = 405,
    
    // File system errors (500-599)
    FILE_SYSTEM_ERROR = 500,
    FILE_NOT_FOUND = 501,
    FILE_READ_ERROR = 502,
    FILE_WRITE_ERROR = 503,
    FILE_PERMISSION_ERROR = 504,
    DISK_FULL = 505,
    INVALID_FILE_FORMAT = 506,
    FILE_CORRUPTED = 507,
    
    // Sample/preset errors (600-699)
    SAMPLE_LOAD_FAILED = 600,
    SAMPLE_FORMAT_UNSUPPORTED = 601,
    SAMPLE_SIZE_TOO_LARGE = 602,
    PRESET_SAVE_FAILED = 603,
    PRESET_LOAD_FAILED = 604,
    PRESET_FORMAT_INVALID = 605,
    SAMPLE_LIBRARY_INIT_FAILED = 606,
    
    // Modulation system errors (700-799)
    MODULATION_INIT_FAILED = 700,
    MODULATION_MATRIX_OVERFLOW = 701,
    MODULATION_SOURCE_INVALID = 702,
    MODULATION_DESTINATION_INVALID = 703,
    LFO_INIT_FAILED = 704,
    ENVELOPE_INIT_FAILED = 705,
    
    // Network/sync errors (800-899)
    NETWORK_INIT_FAILED = 800,
    SYNC_LOST = 801,
    CLOCK_SYNC_ERROR = 802,
    MIDI_SYNC_ERROR = 803,
    
    // Performance/resource errors (900-999)
    CPU_OVERLOAD = 900,
    MEMORY_FRAGMENTATION = 901,
    REAL_TIME_VIOLATION = 902,
    THREAD_PRIORITY_ERROR = 903,
    CACHE_MISS_RATE_HIGH = 904
};

// Error context for debugging and logging
struct ErrorContext {
    ErrorCode code;
    ErrorSeverity severity;
    const char* function;   // Function where error occurred
    const char* file;       // Source file
    int line;              // Line number
    uint32_t timestamp;    // System timestamp
    const char* message;   // Human-readable error message
    void* userData;        // Optional context-specific data
    
    ErrorContext(ErrorCode c, ErrorSeverity s, const char* func, 
                const char* f, int l, const char* msg = nullptr) 
        : code(c), severity(s), function(func), file(f), line(l), 
          timestamp(0), message(msg), userData(nullptr) {}
};

// Error callback types
using ErrorCallback = std::function<void(const ErrorContext& error)>;
using ErrorRecoveryCallback = std::function<bool(const ErrorContext& error)>;

/**
 * Central error handler class
 */
class ErrorHandler {
public:
    static ErrorHandler& getInstance();
    
    // Error reporting
    void reportError(const ErrorContext& error);
    void reportError(ErrorCode code, ErrorSeverity severity = ErrorSeverity::ERROR,
                    const char* message = nullptr);
    
    // Error callbacks
    void setErrorCallback(ErrorCallback callback);
    void setRecoveryCallback(ErrorRecoveryCallback callback);
    
    // Error queries
    ErrorCode getLastError() const { return lastError_; }
    ErrorSeverity getLastErrorSeverity() const { return lastSeverity_; }
    const char* getErrorMessage(ErrorCode code) const;
    uint32_t getErrorCount(ErrorSeverity severity) const;
    
    // Error logging control
    void setLogLevel(ErrorSeverity minSeverity);
    void enableErrorLogging(bool enabled);
    void clearErrorHistory();
    
    // Performance monitoring
    bool isSystemHealthy() const;
    float getErrorRate() const;  // Errors per second
    
    // Recovery support
    bool attemptRecovery(ErrorCode code);
    void registerRecoveryStrategy(ErrorCode code, ErrorRecoveryCallback strategy);
    
private:
    ErrorHandler() = default;
    ErrorCode lastError_ = ErrorCode::SUCCESS;
    ErrorSeverity lastSeverity_ = ErrorSeverity::INFO;
    ErrorCallback errorCallback_;
    ErrorRecoveryCallback recoveryCallback_;
    ErrorSeverity logLevel_ = ErrorSeverity::WARNING;
    bool loggingEnabled_ = true;
    
    // Error statistics
    uint32_t errorCounts_[5] = {0}; // Count per severity level
    uint32_t lastErrorTime_ = 0;
    uint32_t errorRateWindow_ = 0;
};

// Convenience macros for error reporting
#define ETHER_ERROR(code) \
    EtherSynth::ErrorHandler::getInstance().reportError( \
        EtherSynth::ErrorContext(code, EtherSynth::ErrorSeverity::ERROR, \
                                __FUNCTION__, __FILE__, __LINE__))

#define ETHER_ERROR_MSG(code, msg) \
    EtherSynth::ErrorHandler::getInstance().reportError( \
        EtherSynth::ErrorContext(code, EtherSynth::ErrorSeverity::ERROR, \
                                __FUNCTION__, __FILE__, __LINE__, msg))

#define ETHER_WARNING(code) \
    EtherSynth::ErrorHandler::getInstance().reportError( \
        EtherSynth::ErrorContext(code, EtherSynth::ErrorSeverity::WARNING, \
                                __FUNCTION__, __FILE__, __LINE__))

#define ETHER_CRITICAL(code) \
    EtherSynth::ErrorHandler::getInstance().reportError( \
        EtherSynth::ErrorContext(code, EtherSynth::ErrorSeverity::CRITICAL, \
                                __FUNCTION__, __FILE__, __LINE__))

#define ETHER_FATAL(code) \
    EtherSynth::ErrorHandler::getInstance().reportError( \
        EtherSynth::ErrorContext(code, EtherSynth::ErrorSeverity::FATAL, \
                                __FUNCTION__, __FILE__, __LINE__))

// Result type for functions that can fail
template<typename T>
struct Result {
    T value;
    ErrorCode error;
    
    Result(T val) : value(val), error(ErrorCode::SUCCESS) {}
    Result(ErrorCode err) : value{}, error(err) {}
    
    bool isSuccess() const { return error == ErrorCode::SUCCESS; }
    bool isError() const { return error != ErrorCode::SUCCESS; }
    
    // Implicit conversion to bool (success check)
    operator bool() const { return isSuccess(); }
};

// Specialized Result<void> for functions that don't return values
template<>
struct Result<void> {
    ErrorCode error;
    
    Result() : error(ErrorCode::SUCCESS) {}
    Result(ErrorCode err) : error(err) {}
    
    bool isSuccess() const { return error == ErrorCode::SUCCESS; }
    bool isError() const { return error != ErrorCode::SUCCESS; }
    operator bool() const { return isSuccess(); }
};

// Convenience functions for compatibility with existing bool returns
inline bool CheckError(ErrorCode code) {
    if (code != ErrorCode::SUCCESS) {
        ETHER_ERROR(code);
        return false;
    }
    return true;
}

inline ErrorCode BoolToError(bool success, ErrorCode failureCode = ErrorCode::UNKNOWN_ERROR) {
    return success ? ErrorCode::SUCCESS : failureCode;
}

} // namespace EtherSynth