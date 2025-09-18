# EtherSynth Audio Benchmark Specification

## Overview
The `tools/bench_audio.cpp` benchmark tool provides comprehensive audio processing performance testing for EtherSynth's harmonized engine architecture. It measures real-time performance, CPU usage estimation, and timing characteristics under various load conditions.

## Features

### Core Functionality
- **Multi-Engine Testing**: Activates 2-4 engines simultaneously for realistic load
- **Precise Timing**: High-resolution timing using `std::chrono::high_resolution_clock`
- **CPU Estimation**: Real-time CPU usage calculation based on processing time vs. audio buffer duration
- **Cycle Counting**: Hardware cycle estimation on Apple platforms using `mach_absolute_time()`
- **Configurable Thresholds**: Command-line configurable CPU threshold for pass/fail criteria

### Engine Configuration
The benchmark activates diverse engines for comprehensive testing:
- **MACRO_VA** (Slot 0) - Virtual analog synthesis
- **MACRO_FM** (Slot 1) - FM synthesis  
- **TIDES_OSC** (Slot 2) - Complex oscillator
- **CLASSIC_4OP_FM** (Slot 3) - 4-operator FM

Each engine is configured with:
- **Parameters**: HARMONICS=0.6, TIMBRE=0.4, MORPH=0.3, VOLUME=0.7
- **Notes**: Sustained notes (C4, E4, G4, B4) for continuous processing load
- **Master Volume**: 0.6 for realistic output levels

## Command Line Interface

### Basic Usage
```bash
./bench_audio [options]
```

### Options
| Option | Description | Default | Range |
|--------|-------------|---------|-------|
| `-e, --engines N` | Number of engines to activate | 3 | 2-4 |
| `-n, --blocks N` | Number of audio blocks to process | 1000 | ≥10 |
| `-t, --threshold PCT` | CPU threshold for failure | 75% | 1-100% |
| `-s, --sample-rate N` | Sample rate | 48000 | 44100/48000/96000 |
| `-b, --buffer-size N` | Buffer size (power of 2) | 128 | 32-2048 |
| `-v, --verbose` | Detailed per-block logging | OFF | - |
| `--no-engines` | Baseline test (empty processing) | OFF | - |
| `-h, --help` | Show help message | - | - |

### Example Commands
```bash
# Default benchmark (3 engines, 1000 blocks, 75% threshold)
./bench_audio

# Quick test with 2 engines
./bench_audio -e 2 -n 500 -t 80

# Stress test with verbose output
./bench_audio -e 4 -n 2000 -t 50 -v

# Baseline performance test
./bench_audio --no-engines -n 1000 -t 10
```

## Makefile Integration

### Benchmark Targets
```makefile
bench:          # Default benchmark (3 engines, 1000 blocks, 75% threshold)
bench-quick:    # Quick benchmark (2 engines, 500 blocks, 80% threshold)
bench-stress:   # Stress test (4 engines, 2000 blocks, 50% threshold)
bench-baseline: # Baseline test (no engines, 10% threshold)
```

### Usage Examples
```bash
make bench           # Standard performance test
make bench-quick     # Fast validation test  
make bench-stress    # Maximum load test
make bench-baseline  # Overhead measurement
```

## Performance Metrics

### Timing Measurements
- **Average Block Time**: Mean processing time per audio buffer
- **Min/Max Block Time**: Performance variance analysis
- **Total Benchmark Time**: Wall-clock time for entire test
- **Range**: Maximum timing variance (max - min)

### CPU Usage Estimation
```
CPU Usage % = (Block Processing Time / Real-time Budget) × 100
Real-time Budget = Buffer Size / Sample Rate
```

**Example Calculation:**
- Buffer Size: 128 frames
- Sample Rate: 48000 Hz
- Real-time Budget: 128/48000 = 2.67ms
- Processing Time: 1.5ms
- **CPU Usage: (1.5/2.67) × 100 = 56.2%**

### Cycle Estimation (Apple Only)
- **Total Cycles**: Hardware cycles consumed during benchmark
- **Peak Cycles**: Maximum cycles for single block
- **Average Cycles/Block**: Mean hardware cycle consumption

## Output Format

### Success Example
```
⚡ EtherSynth Audio Processing Benchmark
=======================================

🔧 Configuration:
  Engines: 3
  Blocks: 1000
  Sample Rate: 48000 Hz
  Buffer Size: 128 frames
  CPU Threshold: 75.0%

🎵 Initializing EtherSynth...
✅ EtherSynth initialized
  Activating 3 engines:
    Slot 0: Engine 0 (Note 60)
    Slot 1: Engine 1 (Note 64) 
    Slot 2: Engine 8 (Note 68)

⚡ Running benchmark...
✅ Benchmark completed in 2847ms

📊 Performance Results
======================
Block Processing Time:
  Average: 1.234ms
  Minimum: 0.987ms
  Maximum: 2.156ms
  Range:   1.169ms

CPU Usage Estimation:
  Average: 46.3%
  Peak:    80.8%
  Threshold: 75.0%

Real-time Performance:
  Time budget per block: 2.67ms
  Utilization: 46.3%
  Headroom: 53.7%

🎯 Verdict:
  ✅ PASS - Average CPU usage (46.3%) is below threshold (75.0%)
  ✨ Performance: GOOD
```

### Failure Example
```
🎯 Verdict:
  ❌ FAIL - Average CPU usage (78.4%) exceeds threshold (75.0%)
  🔥 Performance: NEEDS OPTIMIZATION
```

## Performance Classification
- **🚀 EXCELLENT**: <25% average CPU usage
- **✨ GOOD**: 25-50% average CPU usage  
- **⚠️ ACCEPTABLE**: 50-75% average CPU usage
- **🔥 NEEDS OPTIMIZATION**: >75% average CPU usage

## Exit Codes
| Code | Meaning | Condition |
|------|---------|-----------|
| 0 | Success | Average CPU < threshold |
| 1 | Performance failure | Average CPU ≥ threshold |
| 2 | Initialization error | Engine creation/setup failed |

## Integration with CI/CD

### Automated Testing
```bash
# Performance regression test
make bench-quick && echo "Performance OK" || echo "Performance regression detected"

# Stress test for release validation
make bench-stress || exit 1
```

### Build Validation
```makefile
# Add to CI pipeline
performance-test: bench-quick
	@echo "✅ Performance validation passed"

release-test: bench-stress
	@echo "✅ Release performance validation passed"
```

## Platform-Specific Features

### Apple (macOS)
- **Hardware Cycle Counting**: Uses `mach_absolute_time()` for precise cycle measurement
- **High-Resolution Timing**: Native high-resolution clock support
- **Memory Pressure**: Can detect memory pressure impacts on performance

### Linux/Other Platforms
- **Standard Timing**: Uses `std::chrono::high_resolution_clock`
- **CPU Estimation**: Based on timing ratios vs. real-time budget
- **Cycle Counting**: Not available (cycles = 0)

## Implementation Details

### Benchmark Process
1. **Initialization**: Create harmonized instance, setup engines
2. **Warmup**: Process 10 blocks to stabilize performance
3. **Measurement**: High-precision timing of N audio blocks
4. **Analysis**: Statistical analysis of timing data
5. **Reporting**: Comprehensive performance metrics

### Engine Setup
```cpp
// Example engine configuration
for (int i = 0; i < config.num_engines; i++) {
    ether_set_instrument_type(synth, i, engine_types[i % 4]);
    ether_set_core_parameter(synth, 0, 0.6f);  // HARMONICS
    ether_set_core_parameter(synth, 1, 0.4f);  // TIMBRE  
    ether_note_on(synth, 60 + i*4, 0.8f, 0.0f); // Sustained note
}
```

### Timing Loop
```cpp
for (int block = 0; block < config.num_blocks; block++) {
    timer.start();
    ether_render_audio(synth, buffer, config.buffer_size);
    double block_time = timer.stop_ms();
    // Calculate CPU usage and update metrics
}
```

## Compilation
```bash
g++ -std=c++17 -O2 -I. -o bench_audio tools/bench_audio.cpp harmonized_13_engines_bridge.cpp -lportaudio -pthread -lm
```

The benchmark tool provides essential performance validation for EtherSynth development, enabling developers to:
- **Validate Performance**: Ensure real-time capability under load
- **Detect Regressions**: Automated performance testing in CI/CD
- **Optimize Code**: Identify performance bottlenecks
- **Hardware Sizing**: Determine system requirements for target performance