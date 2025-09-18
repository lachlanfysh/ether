#pragma once

#include <string>

class SerialPort {
public:
    SerialPort();
    ~SerialPort();

    bool open(const std::string& device);
    int  readData(char* buffer, int maxBytes);
    void close();

private:
    int fd_;
};

