// tools/bench_audio.cpp - EtherSynth Audio Processing Benchmark
// Compile: g++ -std=c++17 -O2 -I. -o bench_audio tools/bench_audio.cpp harmonized_13_engines_bridge.cpp -lportaudio -pthread -lm

#include "../Sources/CEtherSynth/include/EtherSynthBridge.h"
#include <iostream>
#include <chrono>
#include <vector>
#include <cmath>
#include <cstring>
#include <iomanip>

#if defined(__APPLE__)
#include <mach/mach.h>
#include <mach/mach_time.h>
#endif

// Benchmark configuration
struct BenchConfig {
    int num_engines = 3;           // 2-4 engines to activate
    int num_blocks = 1000;         // Number of audio blocks to process
    int sample_rate = 48000;       // Sample rate
    int buffer_size = 128;         // Buffer size (frames)
    float cpu_threshold = 75.0f;   // CPU threshold % for failure
    bool verbose = false;          // Detailed logging
    bool enable_engines = true;    // Actually enable engines vs empty processing
};

// Performance metrics
struct PerfMetrics {
    double total_time_ms = 0.0;
    double min_time_ms = 1e9;
    double max_time_ms = 0.0;
    double avg_cpu_pct = 0.0;
    double peak_cpu_pct = 0.0;
    uint64_t total_cycles = 0;
    uint64_t peak_cycles = 0;
    std::vector<double> block_times;
    std::vector<double> cpu_samples;
};

// High-precision timing
class PrecisionTimer {
public:
    void start() {
        start_time = std::chrono::high_resolution_clock::now();
#if defined(__APPLE__)
        start_cycles = mach_absolute_time();
#endif
    }
    
    double stop_ms() {
        auto end_time = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::nanoseconds>(end_time - start_time);
        return duration.count() / 1000000.0; // Convert to milliseconds
    }
    
    uint64_t stop_cycles() {
#if defined(__APPLE__)
        uint64_t end_cycles = mach_absolute_time();
        static mach_timebase_info_data_t timebase = {0, 0};
        if (timebase.denom == 0) {
            mach_timebase_info(&timebase);
        }
        return (end_cycles - start_cycles) * timebase.numer / timebase.denom;
#else
        return 0; // Cycles not available on this platform
#endif
    }

private:
    std::chrono::high_resolution_clock::time_point start_time;
#if defined(__APPLE__)
    uint64_t start_cycles = 0;
#endif
};

// CPU usage estimation
double estimateCPUUsage(double block_time_ms, int sample_rate, int buffer_size) {
    // Calculate real-time budget per block
    double real_time_budget_ms = (double)buffer_size / sample_rate * 1000.0;
    
    // CPU % = (processing_time / real_time_budget) * 100
    return (block_time_ms / real_time_budget_ms) * 100.0;
}

// Parse command line arguments
BenchConfig parseArgs(int argc, char* argv[]) {
    BenchConfig config;
    
    for (int i = 1; i < argc; i++) {
        std::string arg = argv[i];
        
        if (arg == "-e" || arg == "--engines") {
            if (i + 1 < argc) {
                config.num_engines = std::clamp(std::stoi(argv[++i]), 2, 4);
            }
        } else if (arg == "-n" || arg == "--blocks") {
            if (i + 1 < argc) {
                config.num_blocks = std::max(10, std::stoi(argv[++i]));
            }
        } else if (arg == "-t" || arg == "--threshold") {
            if (i + 1 < argc) {
                config.cpu_threshold = std::clamp(std::stof(argv[++i]), 1.0f, 100.0f);
            }
        } else if (arg == "-s" || arg == "--sample-rate") {
            if (i + 1 < argc) {
                int sr = std::stoi(argv[++i]);
                if (sr == 44100 || sr == 48000 || sr == 96000) {
                    config.sample_rate = sr;
                }
            }
        } else if (arg == "-b" || arg == "--buffer-size") {
            if (i + 1 < argc) {
                int bs = std::stoi(argv[++i]);
                if (bs >= 32 && bs <= 2048 && (bs & (bs - 1)) == 0) { // Power of 2
                    config.buffer_size = bs;
                }
            }
        } else if (arg == "-v" || arg == "--verbose") {
            config.verbose = true;
        } else if (arg == "--no-engines") {
            config.enable_engines = false;
        } else if (arg == "-h" || arg == "--help") {
            std::cout << "EtherSynth Audio Processing Benchmark\n"
                      << "Usage: " << argv[0] << " [options]\n\n"
                      << "Options:\n"
                      << "  -e, --engines N      Number of engines to activate (2-4, default: 3)\n"
                      << "  -n, --blocks N       Number of blocks to process (default: 1000)\n"
                      << "  -t, --threshold PCT  CPU threshold for failure (1-100%, default: 75%)\n"
                      << "  -s, --sample-rate N  Sample rate (44100/48000/96000, default: 48000)\n"
                      << "  -b, --buffer-size N  Buffer size in frames (32-2048, pow2, default: 128)\n"
                      << "  -v, --verbose        Enable detailed logging\n"
                      << "  --no-engines         Test empty processing (baseline)\n"
                      << "  -h, --help           Show this help\n\n"
                      << "Exit codes:\n"
                      << "  0 - Benchmark passed (avg CPU < threshold)\n"
                      << "  1 - Benchmark failed (avg CPU >= threshold)\n"
                      << "  2 - Initialization error\n";
            exit(0);
        }
    }
    
    return config;
}

// Initialize harmonized instance with test engines
bool setupEngines(void* synth, const BenchConfig& config) {
    if (!config.enable_engines) {
        std::cout << "  Engine setup: SKIPPED (baseline test)" << std::endl;
        return true;
    }
    
    // Define a set of diverse engines for realistic load testing
    int engine_types[] = {
        0,  // MACRO_VA
        1,  // MACRO_FM  
        8,  // TIDES_OSC
        12  // CLASSIC_4OP_FM
    };
    
    std::cout << "  Activating " << config.num_engines << " engines:" << std::endl;
    
    for (int i = 0; i < config.num_engines; i++) {
        int slot = i;
        int engine_type = engine_types[i % 4];
        
        // Set instrument type (this creates and initializes the engine)
        ether_set_instrument_type(synth, slot, engine_type);
        
        // Configure engine with some parameters for realistic load
        ether_set_parameter(synth, slot, 0, 0.6f);  // HARMONICS
        ether_set_parameter(synth, slot, 1, 0.4f);  // TIMBRE
        ether_set_parameter(synth, slot, 2, 0.3f);  // MORPH
        ether_set_parameter(synth, slot, 10, 0.7f); // VOLUME
        
        // Set active instrument to this slot for note triggering
        ether_set_active_instrument(synth, slot);
        
        // Trigger a sustained note for processing load
        ether_note_on(synth, 60 + (i * 4), 0.8f, 0.0f); // C4, E4, G4, B4
        
        std::cout << "    Slot " << slot << ": Engine " << engine_type 
                  << " (Note " << (60 + i * 4) << ")" << std::endl;
    }
    
    // Set master volume
    ether_set_master_volume(synth, 0.6f);
    ether_set_bpm(synth, 120.0f);
    
    return true;
}

// Run benchmark with detailed metrics
PerfMetrics runBenchmark(void* synth, const BenchConfig& config) {
    PerfMetrics metrics;
    metrics.block_times.reserve(config.num_blocks);
    metrics.cpu_samples.reserve(config.num_blocks);
    
    // Allocate audio buffer
    std::vector<float> audio_buffer(config.buffer_size * 2); // Stereo
    PrecisionTimer timer;
    
    std::cout << "\n⚡ Running benchmark..." << std::endl;
    std::cout << "  Processing " << config.num_blocks << " blocks of " 
              << config.buffer_size << " frames at " << config.sample_rate << " Hz" << std::endl;
    
    // Warm-up (process a few blocks to stabilize)
    for (int warmup = 0; warmup < 10; warmup++) {
        ether_render_audio(synth, audio_buffer.data(), config.buffer_size);
    }
    
    auto benchmark_start = std::chrono::high_resolution_clock::now();
    
    // Main benchmark loop
    for (int block = 0; block < config.num_blocks; block++) {
        timer.start();
        
        // Process audio block
        ether_render_audio(synth, audio_buffer.data(), config.buffer_size);
        
        double block_time = timer.stop_ms();
        uint64_t block_cycles = timer.stop_cycles();
        
        // Calculate CPU usage for this block
        double cpu_usage = estimateCPUUsage(block_time, config.sample_rate, config.buffer_size);
        
        // Update metrics
        metrics.block_times.push_back(block_time);
        metrics.cpu_samples.push_back(cpu_usage);
        metrics.min_time_ms = std::min(metrics.min_time_ms, block_time);
        metrics.max_time_ms = std::max(metrics.max_time_ms, block_time);
        metrics.peak_cpu_pct = std::max(metrics.peak_cpu_pct, cpu_usage);
        metrics.total_cycles += block_cycles;
        metrics.peak_cycles = std::max(metrics.peak_cycles, block_cycles);
        
        // Verbose logging
        if (config.verbose && (block % 100 == 0 || block < 10)) {
            std::cout << "    Block " << std::setw(4) << block 
                      << ": " << std::fixed << std::setprecision(3) << block_time << "ms"
                      << " (CPU: " << std::setprecision(1) << cpu_usage << "%)" << std::endl;
        }
    }
    
    auto benchmark_end = std::chrono::high_resolution_clock::now();
    auto total_duration = std::chrono::duration_cast<std::chrono::milliseconds>(benchmark_end - benchmark_start);
    metrics.total_time_ms = total_duration.count();
    
    // Calculate averages
    double total_block_time = 0.0;
    double total_cpu = 0.0;
    for (size_t i = 0; i < metrics.block_times.size(); i++) {
        total_block_time += metrics.block_times[i];
        total_cpu += metrics.cpu_samples[i];
    }
    
    double avg_block_time = total_block_time / config.num_blocks;
    metrics.avg_cpu_pct = total_cpu / config.num_blocks;
    
    std::cout << "✅ Benchmark completed in " << metrics.total_time_ms << "ms" << std::endl;
    
    return metrics;
}

// Display comprehensive results
void displayResults(const PerfMetrics& metrics, const BenchConfig& config) {
    std::cout << "\n📊 Performance Results" << std::endl;
    std::cout << "======================" << std::endl;
    
    // Timing metrics
    double avg_block_time = metrics.total_time_ms / config.num_blocks;
    std::cout << "Block Processing Time:" << std::endl;
    std::cout << "  Average: " << std::fixed << std::setprecision(3) << avg_block_time << "ms" << std::endl;
    std::cout << "  Minimum: " << metrics.min_time_ms << "ms" << std::endl;
    std::cout << "  Maximum: " << metrics.max_time_ms << "ms" << std::endl;
    std::cout << "  Range:   " << (metrics.max_time_ms - metrics.min_time_ms) << "ms" << std::endl;
    
    // CPU metrics
    std::cout << "\nCPU Usage Estimation:" << std::endl;
    std::cout << "  Average: " << std::fixed << std::setprecision(1) << metrics.avg_cpu_pct << "%" << std::endl;
    std::cout << "  Peak:    " << metrics.peak_cpu_pct << "%" << std::endl;
    std::cout << "  Threshold: " << config.cpu_threshold << "%" << std::endl;
    
    // Real-time performance
    double real_time_budget = (double)config.buffer_size / config.sample_rate * 1000.0;
    std::cout << "\nReal-time Performance:" << std::endl;
    std::cout << "  Time budget per block: " << std::setprecision(2) << real_time_budget << "ms" << std::endl;
    std::cout << "  Utilization: " << std::setprecision(1) << (avg_block_time / real_time_budget * 100) << "%" << std::endl;
    std::cout << "  Headroom: " << std::setprecision(1) << (100 - metrics.avg_cpu_pct) << "%" << std::endl;
    
    // System info
    std::cout << "\nSystem Configuration:" << std::endl;
    std::cout << "  Engines: " << config.num_engines << (config.enable_engines ? " active" : " disabled") << std::endl;
    std::cout << "  Sample Rate: " << config.sample_rate << " Hz" << std::endl;
    std::cout << "  Buffer Size: " << config.buffer_size << " frames" << std::endl;
    std::cout << "  Total Blocks: " << config.num_blocks << std::endl;
    
    // Cycles information (Apple only)
#if defined(__APPLE__)
    if (metrics.total_cycles > 0) {
        std::cout << "\nCycle Estimation (Apple):" << std::endl;
        std::cout << "  Total Cycles: " << metrics.total_cycles << std::endl;
        std::cout << "  Peak Cycles: " << metrics.peak_cycles << std::endl;
        std::cout << "  Avg Cycles/Block: " << (metrics.total_cycles / config.num_blocks) << std::endl;
    }
#endif
    
    // Performance verdict
    std::cout << "\n🎯 Verdict:" << std::endl;
    if (metrics.avg_cpu_pct < config.cpu_threshold) {
        std::cout << "  ✅ PASS - Average CPU usage (" << std::setprecision(1) << metrics.avg_cpu_pct 
                  << "%) is below threshold (" << config.cpu_threshold << "%)" << std::endl;
    } else {
        std::cout << "  ❌ FAIL - Average CPU usage (" << std::setprecision(1) << metrics.avg_cpu_pct 
                  << "%) exceeds threshold (" << config.cpu_threshold << "%)" << std::endl;
    }
    
    // Performance classification
    if (metrics.avg_cpu_pct < 25.0f) {
        std::cout << "  🚀 Performance: EXCELLENT" << std::endl;
    } else if (metrics.avg_cpu_pct < 50.0f) {
        std::cout << "  ✨ Performance: GOOD" << std::endl;
    } else if (metrics.avg_cpu_pct < 75.0f) {
        std::cout << "  ⚠️  Performance: ACCEPTABLE" << std::endl;
    } else {
        std::cout << "  🔥 Performance: NEEDS OPTIMIZATION" << std::endl;
    }
}

int main(int argc, char* argv[]) {
    std::cout << "⚡ EtherSynth Audio Processing Benchmark" << std::endl;
    std::cout << "=======================================" << std::endl;
    
    // Parse configuration
    BenchConfig config = parseArgs(argc, argv);
    
    std::cout << "\n🔧 Configuration:" << std::endl;
    std::cout << "  Engines: " << config.num_engines << std::endl;
    std::cout << "  Blocks: " << config.num_blocks << std::endl;
    std::cout << "  Sample Rate: " << config.sample_rate << " Hz" << std::endl;
    std::cout << "  Buffer Size: " << config.buffer_size << " frames" << std::endl;
    std::cout << "  CPU Threshold: " << config.cpu_threshold << "%" << std::endl;
    std::cout << "  Verbose: " << (config.verbose ? "ON" : "OFF") << std::endl;
    
    // Initialize EtherSynth
    std::cout << "\n🎵 Initializing EtherSynth..." << std::endl;
    void* synth = ether_create();
    if (!synth) {
        std::cerr << "❌ Failed to create EtherSynth instance" << std::endl;
        return 2;
    }
    
    if (!ether_initialize(synth)) {
        std::cerr << "❌ Failed to initialize EtherSynth" << std::endl;
        ether_destroy(synth);
        return 2;
    }
    
    std::cout << "✅ EtherSynth initialized" << std::endl;
    
    // Setup engines
    if (!setupEngines(synth, config)) {
        std::cerr << "❌ Failed to setup engines" << std::endl;
        ether_shutdown(synth);
        ether_destroy(synth);
        return 2;
    }
    
    // Run benchmark
    PerfMetrics metrics = runBenchmark(synth, config);
    
    // Display results
    displayResults(metrics, config);
    
    // Cleanup
    ether_shutdown(synth);
    ether_destroy(synth);
    
    // Return based on CPU threshold
    return (metrics.avg_cpu_pct >= config.cpu_threshold) ? 1 : 0;
}