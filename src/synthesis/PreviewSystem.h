#pragma once
#include "DSPUtils.h"
#include <memory>
#include <string>
#include <atomic>
#include <thread>
#include <vector>
#include <unordered_map>
#include <mutex>
#include <chrono>

namespace Preview {

/**
 * Card performance grading for adaptive preview system
 */
enum class CardGrade { 
    Gold,   // 3 body streams, 4-item prefetch, 256KB reads
    OK,     // 2 body streams, 2-item prefetch, 128-192KB reads  
    Slow    // 1 body stream, no prefetch, 128KB reads, shorter previews
};

/**
 * RAM pack entry (.bak file format)
 */
struct BakIndexRow {
    uint64_t id;                // Unique sample ID (hash)
    uint32_t stubOfs;           // Offset in stub data
    uint16_t stubLenFrames;     // Length in frames
    uint16_t gainQ15;           // Gain in Q15 format (32768 = 1.0)
    int16_t x, y;               // 2D coordinates for scatter plot
};

/**
 * Preview body entry (.pak file format)
 */
struct PreviewIdx {
    uint64_t id;                // Sample ID
    uint32_t ofs;               // 4KB-aligned offset in pak file
    uint16_t lenMs;             // Length in milliseconds
    uint16_t gainQ15;           // Gain in Q15 format
};

/**
 * RAM pack loader and accessor (.bak files)
 * Loads complete stub collection for instant preview
 */
class ScatterBak {
public:
    ScatterBak();
    ~ScatterBak();
    
    bool load(const char* path);
    void unload();
    
    // RT-safe stub access
    const int16_t* stub(uint64_t id) const;
    size_t stubLength(uint64_t id) const;
    float stubGain(uint64_t id) const;
    bool coords(uint64_t id, int16_t& x, int16_t& y) const;
    uint8_t knn(uint64_t id, uint16_t outIdx[16]) const;
    
    size_t getSampleCount() const { return sampleCount_; }
    bool isLoaded() const { return loaded_; }
    
private:
    bool loaded_;
    size_t sampleCount_;
    
    // Memory-mapped or loaded data
    std::vector<uint8_t> bakData_;
    std::vector<BakIndexRow> index_;
    std::vector<int16_t> stubData_;
    std::unordered_map<uint64_t, size_t> idToIndex_;
    
    // Spatial data structures
    std::vector<std::vector<uint16_t>> spatialBins_;
    std::vector<std::array<uint16_t, 16>> knnData_;
    
    bool parseHeader();
    void buildSpatialIndex();
};

/**
 * SD preview cache (.pak files)
 * Streams preview bodies on-demand with aligned reads
 */
class PreviewCache {
public:
    PreviewCache();
    ~PreviewCache();
    
    void openPak(const char* path);
    void closePak();
    
    void prefetch(uint64_t id);                           // Schedule aligned DMA read
    const int16_t* body(uint64_t id, size_t& nFrames);    // null if not resident
    void setCardGrade(CardGrade grade);
    
    // Statistics
    size_t getCacheSize() const;
    float getHitRate() const;
    
private:
    struct CacheEntry {
        std::vector<int16_t> data;
        std::chrono::steady_clock::time_point lastAccess;
        bool loading;
        
        CacheEntry() : loading(false) {}
    };
    
    struct PendingRead {
        uint64_t id;
        uint32_t offset;
        uint16_t length;
        std::chrono::steady_clock::time_point requestTime;
    };
    
    std::string pakPath_;
    FILE* pakFile_;
    std::vector<PreviewIdx> pakIndex_;
    std::unordered_map<uint64_t, size_t> idToIndex_;
    std::unordered_map<uint64_t, CacheEntry> cache_;
    
    CardGrade cardGrade_;
    size_t maxCacheSize_;
    size_t currentCacheSize_;
    
    // Background loading
    std::thread loadThread_;
    std::atomic<bool> stopLoading_;
    std::vector<PendingRead> pendingReads_;
    std::mutex cacheMutex_;
    std::mutex readMutex_;
    
    // Statistics
    std::atomic<size_t> cacheHits_;
    std::atomic<size_t> cacheMisses_;
    
    void loadingThread();
    void evictLRU();
    bool parsePakHeader();
    void performRead(const PendingRead& read);
};

/**
 * Multi-voice preview player with mixing and voice stealing
 */
class PreviewPlayer {
public:
    static constexpr int MAX_VOICES = 16;
    static constexpr int MAX_BODY_STREAMS = 3;
    
    struct Voice {
        uint64_t id;
        bool active;
        bool usingStub;                           // true = stub, false = body
        const int16_t* data;
        size_t length;
        size_t position;
        float gain;
        float fadeGain;                           // For crossfades
        std::chrono::steady_clock::time_point startTime;
        
        Voice() : id(0), active(false), usingStub(true), data(nullptr), 
                 length(0), position(0), gain(1.0f), fadeGain(1.0f) {}
    };
    
    PreviewPlayer();
    ~PreviewPlayer();
    
    void init(float sampleRate);
    
    // Preview control
    void playStub(uint64_t id);                   // Stage A (instant)
    void bridgeWhenReady(uint64_t id);            // xfade stub→body (Stage B)
    void stopAll();
    void stealOldestIfOverCap();
    
    // Audio rendering
    void renderMix(float* output, size_t frames);
    
    // Configuration
    void setScatterBak(std::shared_ptr<ScatterBak> bak) { scatterBak_ = bak; }
    void setPreviewCache(std::shared_ptr<PreviewCache> cache) { previewCache_ = cache; }
    void setCardGrade(CardGrade grade);
    
private:
    float sampleRate_;
    CardGrade cardGrade_;
    int maxVoices_;
    int maxBodyStreams_;
    
    std::array<Voice, MAX_VOICES> voices_;
    std::shared_ptr<ScatterBak> scatterBak_;
    std::shared_ptr<PreviewCache> previewCache_;
    
    // Mixing
    float softLimiter_;
    static constexpr float LIMITER_THRESHOLD = 0.8f;
    static constexpr float FADE_TIME_MS = 5.0f;
    
    Voice* allocateVoice();
    Voice* findVoice(uint64_t id);
    Voice* findOldestVoice();
    int getActiveVoiceCount() const;
    int getBodyStreamCount() const;
    float calculateFadeGain(Voice& voice, float deltaTime);
    void applySoftLimiter(float* output, size_t frames);
};

/**
 * Preview arbiter - converts UI motion into bounded preview work
 * Implements scribble-proofing and intelligent triggering
 */
class PreviewArbiter {
public:
    PreviewArbiter();
    ~PreviewArbiter();
    
    void init(float sampleRate);
    void setCardGrade(CardGrade grade);
    void setScatterBak(std::shared_ptr<ScatterBak> bak) { scatterBak_ = bak; }
    void setPreviewPlayer(std::shared_ptr<PreviewPlayer> player) { previewPlayer_ = player; }
    
    // Called every 8-10ms from UI thread
    void tick(float x, float y, float timestamp);
    
    // Configuration
    void setSensitivity(float sensitivity) { sensitivity_ = sensitivity; }
    void setMotionThresholds(float slowPx, float fastPx);
    
    // Statistics
    size_t getTriggersPerSecond() const;
    float getPrefetchHitRate() const;
    
private:
    struct MotionHistory {
        float x, y;
        float timestamp;
        float velocity;
    };
    
    float sampleRate_;
    CardGrade cardGrade_;
    std::shared_ptr<ScatterBak> scatterBak_;
    std::shared_ptr<PreviewPlayer> previewPlayer_;
    
    // Motion tracking
    std::vector<MotionHistory> motionHistory_;
    static constexpr size_t MAX_HISTORY = 8;
    
    // Gating parameters
    float minTimeBetweenTriggers_;   // 18-24ms global cap
    float minDistance_;             // Motion threshold
    float lastTriggerTime_;
    float sensitivity_;
    uint64_t lastTriggeredId_;
    
    // Voronoi and similarity suppression
    float voronoiRadius_;
    std::vector<uint64_t> recentlyTriggered_;
    
    // Statistics
    std::atomic<size_t> triggersThisSecond_;
    std::chrono::steady_clock::time_point lastSecondReset_;
    
    // Private methods
    uint64_t findClosestSample(float x, float y);
    float calculateMotionVelocity();
    bool passesGating(uint64_t id, float velocity);
    bool passesVoronoiTest(uint64_t id, float x, float y);
    bool passesSimilarityTest(uint64_t id, float velocity);
    void updateMotionHistory(float x, float y, float timestamp);
    void schedulePrefetch(uint64_t id);
};

/**
 * Complete preview system coordinator
 * Manages all components and provides high-level interface
 */
class PreviewSystem {
public:
    PreviewSystem();
    ~PreviewSystem();
    
    bool init(float sampleRate, const std::string& bakPath, const std::string& pakPath);
    void shutdown();
    
    // UI interaction
    void onMotion(float x, float y);              // Called from UI drag/hover
    void onSelect(uint64_t id);                   // Sample selected for loading
    void stopPreviews();                          // Stop all preview playback
    
    // Audio rendering  
    void renderAudio(float* output, size_t frames);
    
    // Configuration
    void setCardGrade(CardGrade grade);
    void setSensitivity(float sensitivity);
    
    // Status
    bool isReady() const;
    size_t getSampleCount() const;
    
private:
    float sampleRate_;
    bool initialized_;
    CardGrade cardGrade_;
    
    std::shared_ptr<ScatterBak> scatterBak_;
    std::shared_ptr<PreviewCache> previewCache_;
    std::shared_ptr<PreviewPlayer> previewPlayer_;
    std::shared_ptr<PreviewArbiter> arbiter_;
    
    std::chrono::steady_clock::time_point lastTickTime_;
    static constexpr float TICK_INTERVAL_MS = 8.5f;  // 8-10ms cadence
};

} // namespace Preview