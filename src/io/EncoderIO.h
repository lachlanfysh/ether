#pragma once

#include <string>
#include <vector>
#include <functional>

#include "SerialPort.h"

class EncoderIO {
public:
    struct Callbacks {
        std::function<void(int encoderId, int delta)> onTurn;           // E<id>:+/-<n>
        std::function<void(int encoderId, bool pressed)> onButton;      // B<id>:PRESS/RELEASE
    };

    explicit EncoderIO(SerialPort& serial) : serial_(serial) {}

    void setCallbacks(Callbacks cb) { callbacks_ = std::move(cb); }

    // Try to open one of the provided device paths; returns true on first success.
    bool connect(const std::vector<std::string>& devices);

    // Non-blocking poll to read and dispatch events.
    void poll();

private:
    SerialPort& serial_;
    std::string lineBuf_;
    Callbacks callbacks_{};

    void handleLine(const std::string& line);
};

