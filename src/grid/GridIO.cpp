#include "GridIO.h"

#include <iostream>

GridIO::GridIO() {}
GridIO::~GridIO() { stop(); }

bool GridIO::start(int localPort, const std::string& prefix) {
    stop();
    localPort_ = localPort;
    prefix_ = prefix;
    server_ = lo_server_thread_new(std::to_string(localPort_).c_str(), nullptr);
    if (!server_) {
        std::cout << "Failed to create OSC server" << std::endl;
        return false;
    }
    // Basic sys handlers to avoid warnings
    lo_server_thread_add_method(server_, (prefix_ + "/sys/port").c_str(), "", [](const char*,const char*, lo_arg**,int, lo_message, void*)->int{ return 0; }, nullptr);
    lo_server_thread_add_method(server_, (prefix_ + "/sys/host").c_str(), "", [](const char*,const char*, lo_arg**,int, lo_message, void*)->int{ return 0; }, nullptr);
    lo_server_thread_add_method(server_, (prefix_ + "/sys/id").c_str(),   "", [](const char*,const char*, lo_arg**,int, lo_message, void*)->int{ return 0; }, nullptr);
    lo_server_thread_add_method(server_, (prefix_ + "/sys/size").c_str(), "", [](const char*,const char*, lo_arg**,int, lo_message, void*)->int{ return 0; }, nullptr);
    lo_server_thread_add_method(server_, (prefix_ + "/*").c_str(),        nullptr, [](const char*,const char*, lo_arg**,int, lo_message, void*)->int{ return 0; }, nullptr);

    lo_server_thread_start(server_);
    std::cout << "GridIO: listening on port " << localPort_ << std::endl;
    return true;
}

void GridIO::stop() {
    if (server_) {
        lo_server_thread_stop(server_);
        lo_server_thread_free(server_);
        server_ = nullptr;
    }
    if (grid_) {
        lo_address_free(grid_);
        grid_ = nullptr;
    }
    connected_ = false;
}

void GridIO::registerDevice(int devicePort) {
    if (grid_) {
        lo_address_free(grid_);
        grid_ = nullptr;
    }
    grid_ = lo_address_new("127.0.0.1", std::to_string(devicePort).c_str());
    if (!grid_) {
        std::cout << "GridIO: failed to create address for device port " << devicePort << std::endl;
        return;
    }
    lo_send(grid_, "/sys/host",   "s",  "127.0.0.1");
    lo_send(grid_, "/sys/port",   "i",  localPort_);
    lo_send(grid_, "/sys/prefix", "s",  prefix_.c_str());
    lo_send(grid_, "/sys/info",   "");
    connected_ = true;
    std::cout << "GridIO: registered device on port " << devicePort << " prefix " << prefix_ << std::endl;
}

void GridIO::sendLED(int x, int y, int b) {
    if (!grid_) return;
    lo_send(grid_, (prefix_ + "/grid/led/level/set").c_str(), "iii", x, y, b);
}

void GridIO::discover() {
    if (!grid_) grid_ = lo_address_new("127.0.0.1", "12002");
    if (grid_) {
        lo_send(grid_, "/serialosc/list", "si", "127.0.0.1", localPort_);
        lo_send(grid_, "/serialosc/notify", "si", "127.0.0.1", localPort_);
    }
}

int GridIO::serialosc_device_handler_tramp(const char* /*path*/, const char* types, lo_arg** argv, int argc, lo_message /*msg*/, void* user_data) {
    auto* self = static_cast<GridIO*>(user_data);
    if (!self) return 0;
    int port = -1;
    if (argc >= 3) {
        port = argv[2]->i;
    } else if (argc >= 2) {
        port = argv[1]->i;
    }
    if (port > 0) self->registerDevice(port);
    return 0;
}

void GridIO::addSerialOSCHandlers() {
    if (!server_) return;
    lo_server_thread_add_method(server_, "/serialosc/device", "ssi", &GridIO::serialosc_device_handler_tramp, this);
    lo_server_thread_add_method(server_, "/serialosc/add",    "ssi", &GridIO::serialosc_device_handler_tramp, this);
    // Fallback without type checking in case signatures vary
    lo_server_thread_add_method(server_, "/serialosc/device", nullptr, &GridIO::serialosc_device_handler_tramp, this);
    lo_server_thread_add_method(server_, "/serialosc/add",    nullptr, &GridIO::serialosc_device_handler_tramp, this);
}
