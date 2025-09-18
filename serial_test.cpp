#include <iostream>
#include <fcntl.h>
#include <termios.h>
#include <unistd.h>
#include <string>
#include <cstring>
#include <vector>
#include <errno.h>

class SerialPort {
private:
    int fd;
    
public:
    SerialPort() : fd(-1) {}
    
    bool open(const std::string& device, int baudrate = 115200) {
        std::cout << "Attempting to open: " << device << std::endl;
        fd = ::open(device.c_str(), O_RDWR | O_NOCTTY | O_NONBLOCK);
        if (fd == -1) {
            std::cerr << "Failed to open " << device << " - Error: " << strerror(errno) << std::endl;
            return false;
        }
        std::cout << "Successfully opened: " << device << std::endl;
        
        struct termios tty;
        if (tcgetattr(fd, &tty) != 0) {
            std::cerr << "Error getting terminal attributes" << std::endl;
            return false;
        }
        
        // Configure serial port
        cfsetospeed(&tty, B115200);
        cfsetispeed(&tty, B115200);
        
        tty.c_cflag &= ~PARENB;     // No parity
        tty.c_cflag &= ~CSTOPB;     // 1 stop bit
        tty.c_cflag &= ~CSIZE;      // Clear size bits
        tty.c_cflag |= CS8;         // 8 data bits
        tty.c_cflag &= ~CRTSCTS;    // No flow control
        tty.c_cflag |= CREAD | CLOCAL; // Enable reading
        
        tty.c_lflag &= ~ICANON;     // Raw mode
        tty.c_lflag &= ~ECHO;       // No echo
        tty.c_lflag &= ~ECHOE;
        tty.c_lflag &= ~ECHONL;
        tty.c_lflag &= ~ISIG;
        
        tty.c_iflag &= ~(IXON | IXOFF | IXANY); // No software flow control
        tty.c_iflag &= ~(IGNBRK|BRKINT|PARMRK|ISTRIP|INLCR|IGNCR|ICRNL);
        
        tty.c_oflag &= ~OPOST;      // Raw output
        tty.c_oflag &= ~ONLCR;
        
        tty.c_cc[VTIME] = 1;        // 0.1 second timeout
        tty.c_cc[VMIN] = 0;         // Non-blocking
        
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
    
    bool writeData(const std::string& data) {
        if (fd == -1) return false;
        int result = write(fd, data.c_str(), data.length());
        return result == (int)data.length();
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

int main() {
    SerialPort serial;
    
    // Try both USB devices
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
    
    std::cout << "Connected! Listening for encoder data..." << std::endl;
    std::cout << "Sending Ctrl+D to exit REPL mode..." << std::endl;

    // Send Ctrl+D to exit REPL and restart
    serial.writeData("\x04");
    usleep(500000); // Wait 500ms

    std::cout << "Press Ctrl+C to exit" << std::endl;
    
    char buffer[256];
    std::string lineBuffer;
    
    while (true) {
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
                    line.pop_back(); // Remove \r
                }
                
                if (!line.empty()) {
                    std::cout << "Received: " << line << std::endl;
                }
            }
        }
        usleep(10000); // 10ms delay
    }
    
    return 0;
}