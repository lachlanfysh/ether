#include "encoder_control_system.h"
#include <iostream>
#include <string>
#include <sstream>
#include <fcntl.h>
#include <termios.h>
#include <unistd.h>
#include <cstring>
#include <vector>
#include <errno.h>

// Reuse your serial port class
class SerialPort {
private:
    int fd;

public:
    SerialPort() : fd(-1) {}

    bool open(const std::string& device) {
        fd = ::open(device.c_str(), O_RDWR | O_NOCTTY | O_NONBLOCK);
        if (fd == -1) {
            std::cerr << "Failed to open " << device << " - Error: " << strerror(errno) << std::endl;
            return false;
        }

        struct termios tty;
        if (tcgetattr(fd, &tty) != 0) {
            std::cerr << "Error getting terminal attributes" << std::endl;
            return false;
        }

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

        if (tcsetattr(fd, TCSANOW, &tty) != 0) {
            std::cerr << "Error setting terminal attributes" << std::endl;
            return false;
        }

        std::cout << "Opened serial port: " << device << std::endl;
        return true;
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

    ~SerialPort() {
        close();
    }
};

// Demo synthesizer parameters
struct SynthParams {
    float engine1_lpf = 0.5f;
    float engine1_resonance = 0.3f;
    float engine1_volume = 0.7f;
    float engine2_lpf = 0.6f;
    float engine2_resonance = 0.4f;
    float engine2_volume = 0.8f;
    float master_volume = 0.75f;
    float master_reverb = 0.2f;
} synth;

int main() {
    // Initialize encoder control system
    EncoderControlSystem encoders;

    // Register parameters
    encoders.register_parameter("engine1_lpf", "Engine 1 LPF", &synth.engine1_lpf, 0.0f, 1.0f, 0.01f);
    encoders.register_parameter("engine1_res", "Engine 1 Resonance", &synth.engine1_resonance, 0.0f, 1.0f, 0.01f);
    encoders.register_parameter("engine1_vol", "Engine 1 Volume", &synth.engine1_volume, 0.0f, 1.0f, 0.01f);
    encoders.register_parameter("engine2_lpf", "Engine 2 LPF", &synth.engine2_lpf, 0.0f, 1.0f, 0.01f);
    encoders.register_parameter("engine2_res", "Engine 2 Resonance", &synth.engine2_resonance, 0.0f, 1.0f, 0.01f);
    encoders.register_parameter("engine2_vol", "Engine 2 Volume", &synth.engine2_volume, 0.0f, 1.0f, 0.01f);
    encoders.register_parameter("master_vol", "Master Volume", &synth.master_volume, 0.0f, 1.0f, 0.01f);
    encoders.register_parameter("master_rev", "Master Reverb", &synth.master_reverb, 0.0f, 1.0f, 0.01f);

    // Set up callbacks
    encoders.set_menu_callback([](const std::string& param) {
        std::cout << ">>> MENU: " << param << std::endl;
    });

    encoders.set_parameter_callback([](const std::string& param_id, float value) {
        std::cout << ">>> PARAM UPDATE: " << param_id << " = " << value << std::endl;
    });

    encoders.set_latch_callback([](int encoder, const std::string& param_id, bool latched) {
        if (latched) {
            std::cout << ">>> LATCHED: Encoder " << encoder << " -> " << param_id << std::endl;
        } else {
            std::cout << ">>> UNLATCHED: Encoder " << encoder << " (all cleared)" << std::endl;
        }
    });

    // Connect to QT-PY
    SerialPort serial;
    std::vector<std::string> devices = {"/dev/tty.usbmodem101", "/dev/tty.usbmodemm59111127381"};

    bool connected = false;
    for (const auto& device : devices) {
        std::cout << "Trying to connect to: " << device << std::endl;
        if (serial.open(device)) {
            connected = true;
            break;
        }
    }

    if (!connected) {
        std::cout << "Failed to connect to QT-PY device" << std::endl;
        return 1;
    }

    std::cout << "\n=== ENCODER CONTROL DEMO ===" << std::endl;
    std::cout << "Encoder 4: Menu navigation (turn=scroll, press=edit, press again=exit edit)" << std::endl;
    std::cout << "Encoders 1-3: Parameter control (press=latch current param, double-press=clear latches)" << std::endl;
    std::cout << "Press Ctrl+C to exit\n" << std::endl;

    char buffer[256];
    std::string lineBuffer;

    while (true) {
        // Update encoder system (handles double-press timing)
        encoders.update();

        // Read serial data
        int bytesRead = serial.readData(buffer, sizeof(buffer) - 1);
        if (bytesRead > 0) {
            buffer[bytesRead] = '\0';
            lineBuffer += buffer;

            // Process complete lines
            size_t pos;
            while ((pos = lineBuffer.find('\n')) != std::string::npos) {
                std::string line = lineBuffer.substr(0, pos);
                lineBuffer = lineBuffer.substr(pos + 1);

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
        usleep(1000); // 1ms delay
    }

    return 0;
}