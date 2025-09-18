#include "SerialPort.h"

#include <termios.h>
#include <unistd.h>
#include <fcntl.h>

SerialPort::SerialPort() : fd_(-1) {}

SerialPort::~SerialPort() { close(); }

bool SerialPort::open(const std::string& device) {
    fd_ = ::open(device.c_str(), O_RDWR | O_NOCTTY | O_NONBLOCK);
    if (fd_ == -1) return false;

    struct termios tty{};
    if (tcgetattr(fd_, &tty) != 0) return false;

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
    tty.c_cc[VTIME] = 1; // 100ms
    tty.c_cc[VMIN] = 0;

    return tcsetattr(fd_, TCSANOW, &tty) == 0;
}

int SerialPort::readData(char* buffer, int maxBytes) {
    if (fd_ == -1) return -1;
    return static_cast<int>(::read(fd_, buffer, static_cast<size_t>(maxBytes)));
}

void SerialPort::close() {
    if (fd_ != -1) {
        ::close(fd_);
        fd_ = -1;
    }
}

