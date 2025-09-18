// Header-only grid LED state manager with batched flush.
// No OSC or device code here; provide a send callback to flush.

#pragma once

#include <cstdint>
#include <array>
#include <functional>

namespace light {

template<int Width = 16, int Height = 8>
class GridLEDManager {
public:
    static constexpr int kWidth = Width;
    static constexpr int kHeight = Height;

    using Buffer = std::array<std::array<uint8_t, kHeight>, kWidth>;

    GridLEDManager() { clear(); }

    inline void set(int x, int y, uint8_t brightness) noexcept {
        if ((unsigned)x < (unsigned)kWidth && (unsigned)y < (unsigned)kHeight) {
            leds_[x][y] = brightness;
            dirty_ = true;
        }
    }

    inline uint8_t get(int x, int y) const noexcept {
        if ((unsigned)x < (unsigned)kWidth && (unsigned)y < (unsigned)kHeight) {
            return leds_[x][y];
        }
        return 0;
    }

    inline void clear() noexcept {
        for (int x = 0; x < kWidth; ++x) {
            for (int y = 0; y < kHeight; ++y) {
                leds_[x][y] = 0;
            }
        }
        dirty_ = true;
    }

    inline bool dirty() const noexcept { return dirty_; }
    inline void markClean() noexcept { dirty_ = false; }

    template<typename Sender>
    inline void flush(Sender&& send) {
        if (!dirty_) return;
        for (int x = 0; x < kWidth; ++x) {
            for (int y = 0; y < kHeight; ++y) {
                const uint8_t b = leds_[x][y];
                if (b) send(x, y, (int)b);
                else   send(x, y, 0);
            }
        }
        dirty_ = false;
    }

    inline const Buffer& buffer() const noexcept { return leds_; }

private:
    Buffer leds_{};
    bool dirty_{false};
};

} // namespace light

