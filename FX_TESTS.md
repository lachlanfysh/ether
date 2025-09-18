# FX System Test Procedures

## Test Categories

### A. Per-Engine FX Sends (A/B)

#### A1. Send Level Verification
**Purpose**: Verify independent send A/B levels per engine  
**Setup**: Engine playing sustained note, sends initially at 0.0
```cpp
// Test code fragment
ether_set_fx_send(engine, engine_id, FX_SEND_A, 0.8f);
ether_set_fx_send(engine, engine_id, FX_SEND_B, 0.3f);

float send_a = ether_get_fx_send(engine, engine_id, FX_SEND_A);
float send_b = ether_get_fx_send(engine, engine_id, FX_SEND_B);

assert(fabs(send_a - 0.8f) < 0.01f);
assert(fabs(send_b - 0.3f) < 0.01f);
```

**Command Sequence**:
```bash
# Via bridge calls
ether_note_on(engine, 60, 0.8, 0.0)
ether_set_fx_send(engine, 0, 0, 0.8)  # Engine 0, Send A, 80%
ether_set_fx_send(engine, 0, 1, 0.3)  # Engine 0, Send B, 30%
# Verify returns match set values
```

#### A2. Send Independence Test  
**Purpose**: Verify sends don't interfere with each other
```cpp
// Set engine 0 sends
ether_set_fx_send(engine, 0, FX_SEND_A, 1.0f);
ether_set_fx_send(engine, 0, FX_SEND_B, 0.0f);

// Set engine 1 sends differently  
ether_set_fx_send(engine, 1, FX_SEND_A, 0.0f);
ether_set_fx_send(engine, 1, FX_SEND_B, 1.0f);

// Both engines should maintain independent send levels
assert(ether_get_fx_send(engine, 0, FX_SEND_A) == 1.0f);
assert(ether_get_fx_send(engine, 1, FX_SEND_B) == 1.0f);
```

#### A3. RMS Delta with Sends
**Purpose**: Verify sends actually affect FX input levels
```cpp
float rms_dry = measureRMS_1sec(engine, send_a=0.0f, send_b=0.0f);
float rms_send_a = measureRMS_1sec(engine, send_a=1.0f, send_b=0.0f);  
float rms_send_b = measureRMS_1sec(engine, send_a=0.0f, send_b=1.0f);

// FX returns should show different RMS levels
assert(rms_send_a != rms_dry);
assert(rms_send_b != rms_dry);
assert(rms_send_a != rms_send_b);
```

### B. Global FX Parameters

#### B1. Global Parameter Application
**Purpose**: Global FX settings affect all send paths
```bash
# Command sequence
ether_set_global_fx_param(engine, FX_REVERB_SIZE, 0.8)
ether_set_global_fx_param(engine, FX_REVERB_DAMPING, 0.6)

# Test multiple engines with sends active
for engine_id in 0 1 2; do
    ether_set_fx_send(engine, $engine_id, FX_SEND_A, 0.5)
    # All should receive same reverb character
done
```

#### B2. Parameter Range Validation
```cpp  
// Test parameter bounds
ether_set_global_fx_param(engine, FX_REVERB_SIZE, -0.5f); // Invalid
float size = ether_get_global_fx_param(engine, FX_REVERB_SIZE);
assert(size >= 0.0f); // Should clamp to valid range

ether_set_global_fx_param(engine, FX_REVERB_SIZE, 1.5f);  // Invalid  
size = ether_get_global_fx_param(engine, FX_REVERB_SIZE);
assert(size <= 1.0f); // Should clamp to valid range
```

### C. Wet/Dry Balance & Bounds

#### C1. Wet/Dry Level Consistency
**Purpose**: Total output level remains stable across wet/dry mix
```cpp
// Test wet/dry balance at different ratios
struct WetDryTest {
    float dry, wet;
    float expected_total_rms;
};

WetDryTest tests[] = {
    {1.0f, 0.0f, 1.0f},  // All dry
    {0.7f, 0.3f, 1.0f},  // Mixed  
    {0.0f, 1.0f, 1.0f}   // All wet
};

for (auto test : tests) {
    ether_set_fx_wet_dry_mix(engine, test.wet, test.dry);
    float rms = measureRMS_500ms(engine);
    assert(fabs(rms - test.expected_total_rms) < 0.1f);
}
```

#### C2. Send Bounds Checking
```cpp
// Test send level bounds
float test_levels[] = {-1.0f, 0.0f, 0.5f, 1.0f, 2.0f};
float expected[] = {0.0f, 0.0f, 0.5f, 1.0f, 1.0f}; // Clamped

for (int i = 0; i < 5; i++) {
    ether_set_fx_send(engine, 0, FX_SEND_A, test_levels[i]);
    float actual = ether_get_fx_send(engine, 0, FX_SEND_A);
    assert(fabs(actual - expected[i]) < 0.01f);
}
```

### D. Enable/Disable Safety

#### D1. Click-Free FX Disable  
**Purpose**: Disabling FX mid-note doesn't cause audio clicks
```cpp
// Setup: sustained note with FX active
ether_note_on(engine, 60, 0.8f, 0.0f);
ether_set_fx_send(engine, 0, FX_SEND_A, 1.0f);

// Render baseline with FX on
float baseline_rms = 0.0f;
for (int i = 0; i < 10; i++) {
    float buffer[256];
    ether_render_audio(engine, buffer, 256);
    baseline_rms += calculateRMS(buffer, 256);
}

// Disable FX mid-note
ether_set_fx_enable(engine, FX_REVERB, false);

// Check for click artifacts (sudden RMS spike)
float buffer[256];
ether_render_audio(engine, buffer, 256);
float disable_rms = calculateRMS(buffer, 256);

float click_ratio = disable_rms / (baseline_rms / 10.0f);
assert(click_ratio < 3.0f); // No more than 3x RMS spike (6dB)
```

#### D2. FX Enable Ramp-In
```cpp
// Test smooth FX enable
ether_set_fx_enable(engine, FX_DELAY, false);
ether_note_on(engine, 60, 0.8f, 0.0f);

float dry_rms = measureRMS_250ms(engine);

// Enable FX and check for smooth transition  
ether_set_fx_enable(engine, FX_DELAY, true);
ether_set_fx_send(engine, 0, FX_SEND_B, 0.8f);

float wet_rms = measureRMS_250ms(engine);

// Should be different but not discontinuous
assert(fabs(wet_rms - dry_rms) > 0.05f);  // Noticeable change
assert(fabs(wet_rms - dry_rms) < 2.0f);   // But not extreme
```

### E. Parameter Edge Cases

#### E1. Extreme Parameter Values
```bash
# Test FX stability at parameter extremes
echo "Testing reverb extremes..."
ether_set_global_fx_param(engine, FX_REVERB_SIZE, 0.0)    # Minimum
ether_set_global_fx_param(engine, FX_REVERB_DAMPING, 1.0) # Maximum
# Render audio, check for artifacts

echo "Testing delay extremes..."  
ether_set_global_fx_param(engine, FX_DELAY_TIME, 0.001)   # 1ms
ether_set_global_fx_param(engine, FX_DELAY_FEEDBACK, 0.99) # Near oscillation
# Should remain stable, not runaway
```

#### E2. Rapid Parameter Changes
```cpp
// Stress test: rapid parameter modulation
for (int cycle = 0; cycle < 100; cycle++) {
    float mod_value = 0.5f + 0.4f * sinf(cycle * 0.1f);
    ether_set_global_fx_param(engine, FX_REVERB_SIZE, mod_value);
    
    float buffer[64];
    ether_render_audio(engine, buffer, 64);
    
    // Check for instability
    float rms = calculateRMS(buffer, 64);
    assert(rms < 2.0f);  // No runaway
    assert(!std::isnan(rms)); // No NaN values
}
```

### F. Complete FX Test Suite

#### Minimal C++ Test Main
```cpp
// test_fx_system.cpp - Compact FX verification
#include "EtherSynthBridge.h"
#include <iostream>
#include <cassert>

bool test_send_levels(void* engine) {
    ether_note_on(engine, 60, 0.8f, 0.0f);
    
    ether_set_fx_send(engine, 0, 0, 0.8f); // Send A
    ether_set_fx_send(engine, 0, 1, 0.3f); // Send B
    
    float send_a = ether_get_fx_send(engine, 0, 0);
    float send_b = ether_get_fx_send(engine, 0, 1);
    
    bool passed = (fabs(send_a - 0.8f) < 0.01f) && 
                  (fabs(send_b - 0.3f) < 0.01f);
    
    ether_note_off(engine, 60);
    return passed;
}

bool test_wet_dry_balance(void* engine) {
    ether_note_on(engine, 60, 0.8f, 0.0f);
    ether_set_fx_send(engine, 0, 0, 1.0f);
    
    // Test different wet/dry ratios
    float ratios[][2] = {{1.0f, 0.0f}, {0.5f, 0.5f}, {0.0f, 1.0f}};
    
    for (auto& ratio : ratios) {
        ether_set_fx_wet_dry_mix(engine, ratio[1], ratio[0]);
        
        float buffer[256];
        ether_render_audio(engine, buffer, 256);
        float rms = calculateRMS(buffer, 256);
        
        if (rms < 0.01f || rms > 2.0f) return false;
    }
    
    ether_note_off(engine, 60);
    return true;
}

int main() {
    void* engine = ether_create();
    ether_initialize(engine);
    
    bool passed = test_send_levels(engine) && 
                  test_wet_dry_balance(engine);
    
    ether_shutdown(engine);
    ether_destroy(engine);
    
    std::cout << (passed ? "✅ FX tests PASSED" : "❌ FX tests FAILED") << std::endl;
    return passed ? 0 : 1;
}
```

### G. Performance Verification

#### CPU Impact Test
```bash
# Measure FX CPU overhead
echo "Baseline CPU (no FX)..."
baseline_cpu=$(get_cpu_usage)

echo "Enable reverb..."
ether_set_fx_enable(engine, FX_REVERB, true)
reverb_cpu=$(get_cpu_usage)

echo "Enable delay..."  
ether_set_fx_enable(engine, FX_DELAY, true)
delay_cpu=$(get_cpu_usage)

# Calculate FX CPU cost
reverb_cost=$((reverb_cpu - baseline_cpu))
delay_cost=$((delay_cpu - reverb_cpu))

echo "Reverb CPU cost: ${reverb_cost}%"
echo "Delay CPU cost: ${delay_cost}%"

# Assert reasonable limits
test $reverb_cost -lt 10 || echo "❌ Reverb too expensive"
test $delay_cost -lt 5 || echo "❌ Delay too expensive"
```

These tests provide systematic verification of the FX system with clear pass/fail criteria and minimal dependencies.