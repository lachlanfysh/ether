#include <iostream>
#include <thread>
#include <chrono>
#include <lo/lo.h>

// Simple grid hello world test
lo_server_thread osc_server = nullptr;
lo_address grid_addr = nullptr;
bool running = true;
int local_osc_port = 7001; // our OSC listen port
std::string grid_prefix = "/monome"; // OSC prefix we will request

// Forward decl
void register_with_device(int device_port);

// Catch-all logger to see what the device actually sends
int any_msg_logger(const char *path, const char *types, lo_arg **argv, int argc, lo_message /*msg*/, void * /*user_data*/) {
    std::cout << "OSC <= " << (path ? path : "(null)") << " types=" << (types ? types : "") << " argc=" << argc;
    if (argc > 0 && types) {
        std::cout << " vals:";
        for (int i = 0; i < argc; ++i) {
            char t = types[i];
            if (t == 'i') std::cout << " " << argv[i]->i;
            else if (t == 'f') std::cout << " " << argv[i]->f;
            else if (t == 's') std::cout << " '" << &argv[i]->s << "'";
            else std::cout << " (" << t << ")";
        }
    }
    std::cout << std::endl;

    // If this is a serialosc device announcement, extract port and register
    if (path && (std::string(path) == "/serialosc/device" || std::string(path) == "/serialosc/add")) {
        int port = -1;
        if (types && *types) {
            for (int i = argc - 1; i >= 0; --i) {
                if (types[i] == 'i') { port = argv[i]->i; break; }
            }
        }
        if (port < 0 && argc >= 1) {
            port = argv[argc - 1]->i;
        }
        if (port > 0) {
            std::cout << "Registering with device on port " << port << std::endl;
            register_with_device(port);
        }
    }
    return 0;
}

// Handle grid button presses
int grid_key_handler(const char *path, const char *types, lo_arg **argv, int argc, lo_message msg, void *user_data) {
    if (argc >= 3) {
        int x = argv[0]->i;
        int y = argv[1]->i;
        int state = argv[2]->i;
        
        std::cout << "Grid button: x=" << x << " y=" << y << " state=" << state << std::endl;
        
        // Light up the pressed button
        if (grid_addr && state == 1) {
            lo_send(grid_addr, "/monome/grid/led/level/set", "iii", x, y, 15);
            lo_send(grid_addr, "/monome/grid/led/set", "iii", x, y, 1);
        }
        // Turn off when released
        else if (grid_addr && state == 0) {
            lo_send(grid_addr, "/monome/grid/led/level/set", "iii", x, y, 0);
            lo_send(grid_addr, "/monome/grid/led/set", "iii", x, y, 0);
        }
    }
    return 0;
}

// Handle serialosc device announcement: robust port extraction
int serialosc_device_handler(const char* path, const char* types, lo_arg** argv, int argc, lo_message /*msg*/, void* /*user_data*/) {
    std::cout << "serialosc message: " << (path ? path : "") << " types=" << (types ? types : "") << " argc=" << argc << std::endl;
    if (argc <= 0 || !argv) return 0;
    // Try to find the last integer argument as port
    int port = -1;
    if (types && *types) {
        for (int i = argc - 1; i >= 0; --i) {
            if (types[i] == 'i') { port = argv[i]->i; break; }
        }
    }
    if (port < 0 && argc >= 1) {
        // Fallback: assume last arg is int
        port = argv[argc - 1]->i;
    }
    if (port > 0) {
        register_with_device(port);
    }
    return 0;
}

void register_with_device(int device_port) {
    // Free previous address if any
    if (grid_addr) {
        lo_address_free(grid_addr);
        grid_addr = nullptr;
    }

    // Create address to the device's OSC port
    grid_addr = lo_address_new("127.0.0.1", std::to_string(device_port).c_str());
    if (!grid_addr) {
        std::cout << "Error: could not create lo_address for device port " << device_port << std::endl;
        return;
    }

    // Tell the device where to send events (sys messages are NOT prefixed)
    lo_send(grid_addr, "/sys/host", "s", "127.0.0.1");
    lo_send(grid_addr, "/sys/port", "i", local_osc_port);
    lo_send(grid_addr, "/sys/prefix", "s", grid_prefix.c_str());
    lo_send(grid_addr, "/sys/info", "");

    std::cout << "Registered with device on port " << device_port << ", prefix " << grid_prefix << ", listening on " << local_osc_port << std::endl;

    // Give the device a moment to apply settings, then try a test pattern
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    lo_send(grid_addr, (grid_prefix + "/grid/led/level/set").c_str(), "iii", 0, 0, 15);
    lo_send(grid_addr, (grid_prefix + "/grid/led/set").c_str(), "iii", 0, 0, 1);
}

int main() {
    std::cout << "=== Grid Hello World Test ===" << std::endl;
    
    // Create OSC server to receive grid messages
    osc_server = lo_server_thread_new(std::to_string(local_osc_port).c_str(), nullptr);
    if (!osc_server) {
        std::cout << "Error: Could not create OSC server" << std::endl;
        return 1;
    }
    
    // Add handler for grid key messages
    lo_server_thread_add_method(osc_server, (grid_prefix + "/grid/key").c_str(), "iii", grid_key_handler, nullptr);
    // Catch-all to log unexpected paths/types
    lo_server_thread_add_method(osc_server, nullptr, nullptr, any_msg_logger, nullptr);
    // Add handler for serialosc device discovery (accept any types)
    lo_server_thread_add_method(osc_server, "/serialosc/device", nullptr, serialosc_device_handler, nullptr);
    lo_server_thread_add_method(osc_server, "/serialosc/add", nullptr, serialosc_device_handler, nullptr);
    
    // Start OSC server
    lo_server_thread_start(osc_server);
    std::cout << "OSC server started on port 7001" << std::endl;
    
    // Connect to serialosc (default discovery port 12002) and request device list
    lo_address serialosc = lo_address_new("127.0.0.1", "12002");
    if (serialosc) {
        lo_send(serialosc, "/serialosc/list", "si", "127.0.0.1", local_osc_port);
        // Some versions also respond to /serialosc/notify add messages
        lo_send(serialosc, "/serialosc/notify", "si", "127.0.0.1", local_osc_port);
        lo_address_free(serialosc);
    } else {
        std::cout << "Warning: could not create address to serialosc on 127.0.0.1:12002" << std::endl;
    }
    
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    
    std::cout << "Waiting for serialosc device announcement..." << std::endl;
    std::cout << "If your grid is connected, you should see button presses below." << std::endl;
    std::cout << "Press any grid button to test - it should light up!" << std::endl;
    std::cout << "Press Ctrl+C to quit." << std::endl;
    
    // After a short delay, if we've registered to a device, send a test pattern
    if (grid_addr) {
        std::cout << "Sending test pattern to grid..." << std::endl;
        
        // Light up corners briefly
        lo_send(grid_addr, "/monome/grid/led/level/set", "iii", 0, 0, 15);   // Top-left
        lo_send(grid_addr, "/monome/grid/led/set", "iii", 0, 0, 1);
        lo_send(grid_addr, "/monome/grid/led/level/set", "iii", 15, 0, 15);  // Top-right  
        lo_send(grid_addr, "/monome/grid/led/set", "iii", 15, 0, 1);
        lo_send(grid_addr, "/monome/grid/led/level/set", "iii", 0, 7, 15);   // Bottom-left
        lo_send(grid_addr, "/monome/grid/led/set", "iii", 0, 7, 1);
        lo_send(grid_addr, "/monome/grid/led/level/set", "iii", 15, 7, 15);  // Bottom-right
        lo_send(grid_addr, "/monome/grid/led/set", "iii", 15, 7, 1);
        
        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
        
        // Turn off corners
        lo_send(grid_addr, "/monome/grid/led/level/set", "iii", 0, 0, 0);
        lo_send(grid_addr, "/monome/grid/led/set", "iii", 0, 0, 0);
        lo_send(grid_addr, "/monome/grid/led/level/set", "iii", 15, 0, 0);
        lo_send(grid_addr, "/monome/grid/led/set", "iii", 15, 0, 0);
        lo_send(grid_addr, "/monome/grid/led/level/set", "iii", 0, 7, 0);
        lo_send(grid_addr, "/monome/grid/led/set", "iii", 0, 7, 0);
        lo_send(grid_addr, "/monome/grid/led/level/set", "iii", 15, 7, 0);
        lo_send(grid_addr, "/monome/grid/led/set", "iii", 15, 7, 0);
    }
    
    // Keep running until Ctrl+C
    while (running) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    
    // Cleanup
    if (grid_addr) lo_address_free(grid_addr);
    if (osc_server) {
        lo_server_thread_stop(osc_server);
        lo_server_thread_free(osc_server);
    }
    
    std::cout << "Grid hello world test ended." << std::endl;
    return 0;
}
