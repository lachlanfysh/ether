#pragma once
#include "DSPUtils.h"
#include <memory>
#include <string>
#include <atomic>
#include <thread>
#include <vector>
#include <unordered_map>
#include <mutex>

namespace Sample {

/**
 * Sample metadata and format information
 */
struct SampleInfo {
    int sampleRate = 48000;
    int channels = 1;
    int bitDepth = 16;
    size_t totalFrames = 0;
    float durationSeconds = 0.0f;
    bool isValid = false;
    std::string filePath;
    
    // Optional metadata
    float rootNote = 60.0f;  // C4 by default
    float tempo = 120.0f;
    bool hasLoopPoints = false;
    size_t loopStart = 0;
    size_t loopEnd = 0;
};

/**
 * RT-safe ring buffer for streaming audio
 */
class RingBuffer {
public:
    RingBuffer(size_t capacity);
    ~RingBuffer();
    
    // RT-safe operations (lock-free)
    size_t write(const int16_t* data, size_t frames);
    size_t read(int16_t* dest, size_t frames);
    size_t available() const;
    size_t space() const;
    void reset();
    
private:
    std::vector<int16_t> buffer_;
    std::atomic<size_t> writePos_{0};
    std::atomic<size_t> readPos_{0};
    size_t capacity_;
    size_t mask_;  // For power-of-2 sized buffers
    
    size_t nextPowerOf2(size_t n);
};

/**
 * 4-point Lagrange resampler for high-quality pitch shifting
 */
class LagrangeResampler {
public:
    LagrangeResampler();
    ~LagrangeResampler();
    
    void setPitchRatio(float ratio);  // 1.0 = no change, 2.0 = octave up, 0.5 = octave down
    void reset();
    
    // Process samples with resampling
    void process(const int16_t* input, size_t inputFrames, 
                 int16_t* output, size_t& outputFrames, bool loop = false);
    
private:
    float ratio_;
    double phase_;
    int16_t history_[4];  // Previous 4 samples for interpolation
    bool initialized_;
    
    // 4-point Lagrange interpolation
    int16_t interpolate(const int16_t* samples, double frac) const;
};

/**
 * WAV file format parser and loader
 */
class WavLoader {
public:
    static bool loadSampleInfo(const std::string& filePath, SampleInfo& info);
    static bool loadToRAM(const std::string& filePath, std::vector<int16_t>& buffer, SampleInfo& info);
    static bool openForStreaming(const std::string& filePath, SampleInfo& info, void*& fileHandle);
    static size_t readFrames(void* fileHandle, int16_t* buffer, size_t frames);
    static void closeFile(void* fileHandle);
    
private:
    struct WavHeader {
        char riff[4];
        uint32_t fileSize;
        char wave[4];
        char fmt[4];
        uint32_t fmtSize;
        uint16_t format;
        uint16_t channels;
        uint32_t sampleRate;
        uint32_t byteRate;
        uint16_t blockAlign;
        uint16_t bitsPerSample;
        char data[4];
        uint32_t dataSize;
    };
    
    static bool parseHeader(FILE* file, WavHeader& header, SampleInfo& info);
    static void convertTo16Bit(const uint8_t* input, int16_t* output, size_t frames, 
                              int channels, int bitDepth);
};

/**
 * LRU cache for frequently accessed samples
 */
class SampleCache {
public:
    SampleCache(size_t maxSizeBytes = 64 * 1024 * 1024);  // 64MB default
    ~SampleCache();
    
    bool get(const std::string& key, std::vector<int16_t>& buffer, SampleInfo& info);
    void put(const std::string& key, const std::vector<int16_t>& buffer, const SampleInfo& info);
    void clear();
    size_t getCurrentSize() const { return currentSize_; }
    size_t getMaxSize() const { return maxSize_; }
    
private:
    struct CacheEntry {
        std::vector<int16_t> buffer;
        SampleInfo info;
        std::chrono::steady_clock::time_point lastAccess;
        size_t size;
    };
    
    std::unordered_map<std::string, CacheEntry> cache_;
    mutable std::mutex cacheMutex_;
    size_t maxSize_;
    size_t currentSize_;
    
    void evictLRU();
    size_t calculateSize(const std::vector<int16_t>& buffer) const;
};

/**
 * Main sample buffer class - handles both RAM and streaming modes
 */
class SampleBuffer {
public:
    enum class Mode {
        RAM,        // Entire sample loaded in RAM
        STREAMING   // Sample streamed from disk with ring buffer
    };
    
    SampleBuffer();
    ~SampleBuffer();
    
    // Load sample (automatically chooses RAM vs streaming based on size)
    bool load(const std::string& filePath, float threshold1MB = 1.0f);
    void unload();
    
    // Playback interface
    void startPlayback(float startPosition = 0.0f, bool loop = false);
    void stopPlayback();
    void setPitch(float semitones);  // -24 to +24 semitones
    void setPosition(float position);  // 0.0 to 1.0
    
    // RT-safe sample generation
    void renderSamples(int16_t* output, size_t frames, float gain = 1.0f);
    
    // Sample information
    const SampleInfo& getInfo() const { return info_; }
    bool isLoaded() const { return loaded_; }
    bool isPlaying() const { return playing_; }
    Mode getMode() const { return mode_; }
    float getPosition() const;
    
    // Preview system support
    bool loadStub(const int16_t* stubData, size_t stubFrames);  // For .bak preview stubs
    bool loadPreview(const int16_t* previewData, size_t previewFrames);  // For .pak preview bodies
    
    // Static cache management
    static void setCacheInstance(std::shared_ptr<SampleCache> cache);
    static std::shared_ptr<SampleCache> getCacheInstance();
    
private:
    SampleInfo info_;
    Mode mode_;
    bool loaded_;
    std::atomic<bool> playing_;
    std::atomic<bool> loop_;
    
    // RAM mode
    std::vector<int16_t> ramBuffer_;
    std::atomic<size_t> playPosition_;
    
    // Streaming mode
    std::unique_ptr<RingBuffer> ringBuffer_;
    void* fileHandle_;
    std::thread streamThread_;
    std::atomic<bool> stopStreaming_;
    
    // Resampling
    std::unique_ptr<LagrangeResampler> resampler_;
    float pitchRatio_;
    
    // Preview data (for instant audition)
    std::vector<int16_t> stubBuffer_;
    std::vector<int16_t> previewBuffer_;
    bool hasStub_;
    bool hasPreview_;
    
    // Static cache
    static std::shared_ptr<SampleCache> globalCache_;
    
    // Private methods
    void streamingThread();
    void updatePitchRatio(float semitones);
    size_t renderRAM(int16_t* output, size_t frames, float gain);
    size_t renderStreaming(int16_t* output, size_t frames, float gain);
    size_t renderPreview(int16_t* output, size_t frames, float gain, bool useStub);
};

/**
 * Voice allocator with choke groups for sampler engines
 */
class VoiceAllocator {
public:
    static constexpr int MAX_VOICES = 64;
    static constexpr int MAX_CHOKE_GROUPS = 8;
    
    struct Voice {
        int id;
        int pad;  // Which pad/key triggered this voice
        int chokeGroup;
        float velocity;
        bool active;
        std::chrono::steady_clock::time_point startTime;
        std::unique_ptr<SampleBuffer> sampleBuffer;
    };
    
    VoiceAllocator();
    ~VoiceAllocator();
    
    // Voice management
    Voice* allocateVoice(int pad, int chokeGroup, float velocity);
    void releaseVoice(Voice* voice);
    void chokeGroup(int chokeGroup);
    void chokePad(int pad);  // For "Cut Self" mode
    
    // Voice access
    Voice* getVoice(int index) { return (index >= 0 && index < MAX_VOICES) ? &voices_[index] : nullptr; }
    int getActiveVoiceCount() const;
    
    // Configuration
    void setMaxVoicesPerPad(int pad, int maxVoices);
    
    enum class Priority { LAST, OLDEST, HIGH_VEL };
    void setVoicePriority(Priority priority) { priority_ = priority; }
    
private:
    Voice voices_[MAX_VOICES];
    int maxVoicesPerPad_[25];  // Max voices per pad (for 25 pads)
    Priority priority_;
    
    Voice* findOldestVoice();
    Voice* findLowestVelocityVoice();
    int countVoicesForPad(int pad) const;
};

} // namespace Sample