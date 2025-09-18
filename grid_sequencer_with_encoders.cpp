// This will be a modified version of grid_sequencer.cpp with encoder integration
// I'll create a patch-style integration that adds the encoder system

// Add these includes near the top after the existing includes:
#include "encoder_control_system.h"
#include <fcntl.h>
#include <termios.h>
#include <unistd.h>
#include <cstring>
#include <vector>
#include <errno.h>

// Add this serial port class (reuse from our demo)
class SerialPort {
private:
    int fd;
public:
    SerialPort() : fd(-1) {}

    bool open(const std::string& device) {
        fd = ::open(device.c_str(), O_RDWR | O_NOCTTY | O_NONBLOCK);
        if (fd == -1) return false;

        struct termios tty;
        if (tcgetattr(fd, &tty) != 0) return false;

        cfsetospeed(&tty, B115200);
        cfsetispeed(&tty, B115200);
        tty.c_cflag &= ~PARENB;
        tty.c_cflag &= ~CSTOPB;
        tty.c_cflag &= ~CSIZE;
        tty.c_cflag |= CS8;
        tty.c_cflag &= ~CRTSCTS;
        tty.c_cflag |= CREAD | CLOCAL;
        tty.c_lflag &= ~ICANON;
        tty.c_lflag &= ~ECHO;
        tty.c_lflag &= ~ECHOE;
        tty.c_lflag &= ~ECHONL;
        tty.c_lflag &= ~ISIG;
        tty.c_iflag &= ~(IXON | IXOFF | IXANY);
        tty.c_iflag &= ~(IGNBRK|BRKINT|PARMRK|ISTRIP|INLCR|IGNCR|ICRNL);
        tty.c_oflag &= ~OPOST;
        tty.c_oflag &= ~ONLCR;
        tty.c_cc[VTIME] = 1;
        tty.c_cc[VMIN] = 0;

        return tcsetattr(fd, TCSANOW, &tty) == 0;
    }

    int readData(char* buffer, int maxBytes) {
        if (fd == -1) return -1;
        return read(fd, buffer, maxBytes);
    }

    void close() {
        if (fd != -1) {
            ::close(fd);
            fd = -1;
        }
    }

    ~SerialPort() { close(); }
};

/*
INTEGRATION INSTRUCTIONS:

1. Add the above includes and SerialPort class to your grid_sequencer.cpp

2. In the GridSequencer class (around line 939), add these member variables:

    // Encoder control system
    EncoderControlSystem encoders;
    SerialPort encoderSerial;
    std::string serialLineBuffer;

3. In the GridSequencer constructor (around line 949), add:

    // Initialize encoder control system
    setupEncoderSystem();

4. Add this method to GridSequencer class:

*/

void setupEncoderSystem() {
    // Register all engine parameters for all engines
    for (int engine = 0; engine < MAX_ENGINES; ++engine) {
        std::string prefix = "engine" + std::to_string(engine) + "_";

        // Register each parameter
        encoders.register_parameter(
            prefix + "cutoff", "Engine " + std::to_string(engine) + " Cutoff",
            &engineParameters[engine][static_cast<int>(ParameterID::CUTOFF)], 0.0f, 1.0f, 0.01f);

        encoders.register_parameter(
            prefix + "resonance", "Engine " + std::to_string(engine) + " Resonance",
            &engineParameters[engine][static_cast<int>(ParameterID::RESONANCE)], 0.0f, 1.0f, 0.01f);

        encoders.register_parameter(
            prefix + "attack", "Engine " + std::to_string(engine) + " Attack",
            &engineParameters[engine][static_cast<int>(ParameterID::ATTACK)], 0.0f, 1.0f, 0.01f);

        encoders.register_parameter(
            prefix + "decay", "Engine " + std::to_string(engine) + " Decay",
            &engineParameters[engine][static_cast<int>(ParameterID::DECAY)], 0.0f, 1.0f, 0.01f);

        encoders.register_parameter(
            prefix + "sustain", "Engine " + std::to_string(engine) + " Sustain",
            &engineParameters[engine][static_cast<int>(ParameterID::SUSTAIN)], 0.0f, 1.0f, 0.01f);

        encoders.register_parameter(
            prefix + "release", "Engine " + std::to_string(engine) + " Release",
            &engineParameters[engine][static_cast<int>(ParameterID::RELEASE)], 0.0f, 1.0f, 0.01f);

        encoders.register_parameter(
            prefix + "volume", "Engine " + std::to_string(engine) + " Volume",
            &engineParameters[engine][static_cast<int>(ParameterID::VOLUME)], 0.0f, 1.0f, 0.01f);

        encoders.register_parameter(
            prefix + "pan", "Engine " + std::to_string(engine) + " Pan",
            &engineParameters[engine][static_cast<int>(ParameterID::PAN)], 0.0f, 1.0f, 0.01f);

        encoders.register_parameter(
            prefix + "reverb", "Engine " + std::to_string(engine) + " Reverb",
            &engineParameters[engine][static_cast<int>(ParameterID::REVERB_MIX)], 0.0f, 1.0f, 0.01f);
    }

    // Set up callbacks
    encoders.set_parameter_callback([this](const std::string& param_id, float value) {
        // Update the actual engine when encoder changes parameter
        updateEngineFromEncoderChange(param_id, value);
    });

    encoders.set_menu_callback([this](const std::string& param_id) {
        // Sync the main UI navigation with encoder navigation
        syncMenuWithEncoder(param_id);
    });

    encoders.set_latch_callback([this](int encoder_id, const std::string& param_id, bool latched) {
        if (latched) {
            std::cout << "🔒 Encoder " << encoder_id << " latched to " << param_id << std::endl;
        } else {
            std::cout << "🔓 Encoder " << encoder_id << " latches cleared" << std::endl;
        }
    });

    // Connect to QT-PY
    std::vector<std::string> devices = {"/dev/tty.usbmodem101", "/dev/tty.usbmodemm59111127381"};
    for (const auto& device : devices) {
        if (encoderSerial.open(device)) {
            std::cout << "📡 Connected to encoder controller: " << device << std::endl;
            break;
        }
    }
}

void updateEngineFromEncoderChange(const std::string& param_id, float value) {
    // Parse param_id like "engine2_cutoff"
    size_t underscore = param_id.find('_');
    if (underscore == std::string::npos) return;

    std::string engine_part = param_id.substr(0, underscore);
    std::string param_part = param_id.substr(underscore + 1);

    if (engine_part.substr(0, 6) != "engine") return;

    int engine_num = std::stoi(engine_part.substr(6));
    if (engine_num < 0 || engine_num >= MAX_ENGINES) return;

    // Map parameter name to ParameterID
    ParameterID pid;
    if (param_part == "cutoff") pid = ParameterID::CUTOFF;
    else if (param_part == "resonance") pid = ParameterID::RESONANCE;
    else if (param_part == "attack") pid = ParameterID::ATTACK;
    else if (param_part == "decay") pid = ParameterID::DECAY;
    else if (param_part == "sustain") pid = ParameterID::SUSTAIN;
    else if (param_part == "release") pid = ParameterID::RELEASE;
    else if (param_part == "volume") pid = ParameterID::VOLUME;
    else if (param_part == "pan") pid = ParameterID::PAN;
    else if (param_part == "reverb") pid = ParameterID::REVERB_MIX;
    else return;

    // Update the actual synthesizer engine
    int slot = rowToSlot[engine_num];
    if (slot < 0) slot = 0;
    ether_set_instrument_parameter(etherEngine, slot, static_cast<int>(pid), value);
}

void syncMenuWithEncoder(const std::string& param_id) {
    // This syncs the grid sequencer's parameter selection with encoder menu navigation
    // You could update selectedParamIndex here to match encoder selection
    // For now, just show what the encoder selected
    std::cout << "🎛️ Encoder menu: " << param_id << std::endl;
}

void processEncoderInput() {
    char buffer[256];
    int bytesRead = encoderSerial.readData(buffer, sizeof(buffer) - 1);
    if (bytesRead > 0) {
        buffer[bytesRead] = '\0';
        serialLineBuffer += buffer;

        // Process complete lines
        size_t pos;
        while ((pos = serialLineBuffer.find('\n')) != std::string::npos) {
            std::string line = serialLineBuffer.substr(0, pos);
            serialLineBuffer = serialLineBuffer.substr(pos + 1);

            if (!line.empty() && line.back() == '\r') {
                line.pop_back();
            }

            // Parse encoder commands
            if (!line.empty() && line[0] == 'E') {
                // Format: E1:+1 or E2:-1
                if (line.size() >= 4 && line[2] == ':') {
                    int encoder_id = line[1] - '0';
                    int delta = std::stoi(line.substr(3));
                    encoders.handle_encoder_turn(encoder_id, delta);
                }
            } else if (!line.empty() && line[0] == 'B') {
                // Format: B1:PRESS or B1:RELEASE
                if (line.size() >= 4 && line[2] == ':') {
                    int encoder_id = line[1] - '0';
                    std::string action = line.substr(3);
                    if (action == "PRESS") {
                        encoders.handle_button_press(encoder_id);
                    } else if (action == "RELEASE") {
                        encoders.handle_button_release(encoder_id);
                    }
                }
            }
        }
    }
}

/*
5. In the main run() loop (around line 1458), add this call at the beginning of the while loop:

        // Process encoder input
        processEncoderInput();
        encoders.update();

6. Compile with:
   g++ -std=c++17 -o grid_sequencer_with_encoders grid_sequencer.cpp encoder_control_system.cpp -lportaudio -llo

That's it! Your grid sequencer will now have:

- Encoder 4: Navigate through ALL engine parameters across all engines
- Encoder 4 press: Enter edit mode for direct parameter adjustment
- Encoders 1-3: Latch to any parameter for global control
- Single press: Latch current parameter
- Double press: Clear all latches

The encoder system will seamlessly integrate with your existing keyboard controls.
*/