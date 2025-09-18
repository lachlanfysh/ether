#include <iostream>
#include <lo/lo.h>
#include <thread>
#include <chrono>

int main() {
    std::cout << "Testing OSC communication with monome grid..." << std::endl;
    
    // Try to connect to serialosc
    lo_address serialosc = lo_address_new("127.0.0.1", "12002");
    
    // Create our server to receive replies
    lo_server_thread server = lo_server_thread_new("7001", nullptr);
    lo_server_thread_start(server);
    
    std::cout << "Listening on port 7001..." << std::endl;
    
    // Send list request to serialosc
    std::cout << "Sending /serialosc/list to 127.0.0.1:12002..." << std::endl;
    int result = lo_send(serialosc, "/serialosc/list", "si", "127.0.0.1", 7001);
    
    if (result == -1) {
        std::cout << "Failed to send OSC message" << std::endl;
    } else {
        std::cout << "OSC message sent successfully" << std::endl;
    }
    
    std::cout << "Waiting for replies..." << std::endl;
    std::this_thread::sleep_for(std::chrono::seconds(2));
    
    // Try direct grid connection (common port)
    lo_address grid = lo_address_new("127.0.0.1", "8080");
    std::cout << "Testing direct grid connection on port 8080..." << std::endl;
    result = lo_send(grid, "/monome/grid/led/all", "i", 5);
    
    if (result == -1) {
        std::cout << "Failed to send to grid on 8080" << std::endl;
    } else {
        std::cout << "Grid LED message sent to 8080" << std::endl;
    }
    
    std::this_thread::sleep_for(std::chrono::seconds(1));
    
    // Try port 8000
    lo_address_free(grid);
    grid = lo_address_new("127.0.0.1", "8000");
    std::cout << "Testing direct grid connection on port 8000..." << std::endl;
    result = lo_send(grid, "/monome/grid/led/all", "i", 5);
    
    if (result == -1) {
        std::cout << "Failed to send to grid on 8000" << std::endl;
    } else {
        std::cout << "Grid LED message sent to 8000" << std::endl;
    }
    
    std::this_thread::sleep_for(std::chrono::seconds(1));
    
    // Clean up
    lo_address_free(serialosc);
    lo_address_free(grid);
    lo_server_thread_stop(server);
    lo_server_thread_free(server);
    
    std::cout << "OSC test complete" << std::endl;
    return 0;
}