// Header-only parameter cache for UI reads.
// Design: write-through from UI/control thread, atomic reads for rendering.

#pragma once

#include <atomic>
#include <cstdint>
#include <unordered_map>
#include <mutex>
#include <utility>

namespace light {

// Cheap wrapper for atomic float with relaxed operations suitable for UI reads.
struct AtomicFloat {
    std::atomic<float> v{0.0f};
    AtomicFloat() noexcept = default;
    // Provide copy semantics using relaxed load/store to allow storage in containers.
    AtomicFloat(const AtomicFloat& other) noexcept { v.store(other.v.load(std::memory_order_relaxed), std::memory_order_relaxed); }
    AtomicFloat& operator=(const AtomicFloat& other) noexcept {
        if (this != &other) v.store(other.v.load(std::memory_order_relaxed), std::memory_order_relaxed);
        return *this;
    }
    // Move same as copy (atomic has no true move semantics)
    AtomicFloat(AtomicFloat&& other) noexcept { v.store(other.v.load(std::memory_order_relaxed), std::memory_order_relaxed); }
    AtomicFloat& operator=(AtomicFloat&& other) noexcept {
        if (this != &other) v.store(other.v.load(std::memory_order_relaxed), std::memory_order_relaxed);
        return *this;
    }
    inline void store(float x) noexcept { v.store(x, std::memory_order_relaxed); }
    inline float load() const noexcept { return v.load(std::memory_order_relaxed); }
};

// Key: (instrument << 16) | paramId
inline constexpr uint32_t keyFor(int instrument, int paramId) noexcept {
    return (uint32_t)((instrument & 0xFFFF) << 16) | (uint32_t)(paramId & 0xFFFF);
}

class ParameterCache {
public:
    // Store without allocation on hot path: buckets reserved once.
    void reserve(size_t n) {
        std::lock_guard<std::mutex> lock(m_);
        map_.reserve(n);
    }

    void set(int instrument, int paramId, float value) {
        const uint32_t k = keyFor(instrument, paramId);
        std::lock_guard<std::mutex> lock(m_);
        auto it = map_.find(k);
        if (it == map_.end()) it = map_.emplace(k, AtomicFloat{}).first;
        it->second.store(value);
    }

    // Returns true if present; outputs value via out param.
    bool get(int instrument, int paramId, float& out) const {
        const uint32_t k = keyFor(instrument, paramId);
        std::lock_guard<std::mutex> lock(m_);
        auto it = map_.find(k);
        if (it == map_.end()) return false;
        out = it->second.load();
        return true;
    }

    // Read with fallback to provided default.
    float getOr(int instrument, int paramId, float def) const {
        float v{};
        return get(instrument, paramId, v) ? v : def;
    }

private:
    // Note: This map is read-mostly after initial fill. If you need full RT-safety
    // for writes from multiple threads, move to SPSC queue or double-buffered maps.
    std::unordered_map<uint32_t, AtomicFloat> map_;
    mutable std::mutex m_{};
};

} // namespace light
