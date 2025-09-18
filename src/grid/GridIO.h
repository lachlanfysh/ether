#pragma once

#include <string>
#include <functional>
#include <lo/lo.h>

class GridIO {
public:
    GridIO();
    ~GridIO();

    // Start OSC server on given local port and set prefix (e.g. "/monome").
    bool start(int localPort, const std::string& prefix);
    void stop();

    // Register the grid device discovered on a specific port (from serialosc).
    void registerDevice(int devicePort);

    // Send a single LED level set.
    void sendLED(int x, int y, int b);

    // Trigger serialosc discovery (sends /serialosc/list and /serialosc/notify)
    void discover();

    // Accessors
    lo_server_thread server() const { return server_; }
    bool connected() const { return connected_; }
    const std::string& prefix() const { return prefix_; }

    // Install serialosc device handlers onto an existing server (if not using start()).
    void addSerialOSCHandlers();

private:
    lo_server_thread server_ = nullptr;
    lo_address grid_ = nullptr;
    std::string prefix_ = "/monome";
    int localPort_ = 7001;
    bool connected_ = false;

    // trampolines
    static int serialosc_device_handler_tramp(const char* path, const char* types, lo_arg** argv, int argc, lo_message msg, void* user_data);
};
