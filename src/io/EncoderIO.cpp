#include "EncoderIO.h"

#include <cstdlib>

bool EncoderIO::connect(const std::vector<std::string>& devices) {
    for (const auto& dev : devices) {
        if (serial_.open(dev)) {
            return true;
        }
    }
    return false;
}

void EncoderIO::poll() {
    char buffer[256];
    int bytesRead = serial_.readData(buffer, sizeof(buffer) - 1);
    if (bytesRead <= 0) return;
    buffer[bytesRead] = '\0';
    lineBuf_ += buffer;

    for (;;) {
        size_t pos = lineBuf_.find('\n');
        if (pos == std::string::npos) break;
        std::string line = lineBuf_.substr(0, pos);
        lineBuf_.erase(0, pos + 1);
        if (!line.empty() && line.back() == '\r') line.pop_back();
        if (!line.empty()) handleLine(line);
    }
}

void EncoderIO::handleLine(const std::string& line) {
    if (line.empty()) return;
    if (line[0] == 'E') {
        // Format: E1:+1 or E2:-1
        if (line.size() >= 4 && line[2] == ':') {
            int encoder_id = line[1] - '0';
            int delta = std::atoi(line.c_str() + 3);
            if (callbacks_.onTurn) callbacks_.onTurn(encoder_id, delta);
        }
    } else if (line[0] == 'B') {
        // Format: B1:PRESS or B1:RELEASE
        if (line.size() >= 4 && line[2] == ':') {
            int encoder_id = line[1] - '0';
            const char* action = line.c_str() + 3;
            if (callbacks_.onButton) {
                if (std::strncmp(action, "PRESS", 5) == 0) callbacks_.onButton(encoder_id, true);
                else if (std::strncmp(action, "RELEASE", 7) == 0) callbacks_.onButton(encoder_id, false);
            }
        }
    }
}

