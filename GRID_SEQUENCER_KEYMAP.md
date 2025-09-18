# Grid Sequencer Key Map & Workflows

## Physical Layout (16x16 Grid + Function Buttons)

### Grid Layout
```
Function Row:  [SHIFT] [ENGINE] [PATTERN] [FX] [LFO] [PLAY] [WRITE] [COPY] [DELETE] [...] 

Step Grid:     1  2  3  4  5  6  7  8  9  10 11 12 13 14 15 16
Row 1 (Kick):  ■  □  ■  □  ■  □  ■  □  ■  □  ■  □  ■  □  ■  □
Row 2 (Snare): □  □  ■  □  □  □  ■  □  □  □  ■  □  □  □  ■  □  
Row 3 (HiHat): ■  ■  ■  ■  ■  ■  ■  ■  ■  ■  ■  ■  ■  ■  ■  ■
...
Row 16:        □  □  □  □  □  □  □  □  □  □  □  □  □  □  □  □

Track Select:  [T1] [T2] [T3] [T4] [T5] [T6] [T7] [T8] [T9] ... [T16]
```

## Function Buttons

### Core Transport
| Button | Action | LED | Description |
|--------|--------|-----|-------------|
| **PLAY** | Toggle playback | Green = playing | Start/stop sequencer |
| **WRITE** | Toggle write mode | Red = writing | Enable step recording |

### Pattern Management  
| Button | Action | Modifier | Description |
|--------|--------|----------|-------------|
| **PATTERN** | Pattern select | Hold + step = select | Access pattern A1-P16 |
| **COPY** | Copy pattern | Hold + pattern = copy | Duplicate current pattern |
| **DELETE** | Clear pattern | Hold + pattern = clear | Erase pattern data |

### Engine & Sound
| Button | Action | Modifier | Description |
|--------|--------|----------|-------------|
| **ENGINE** | Engine select | Hold + track = assign | Change synthesis engine |
| **FX** | FX page | Hold + knob = edit | Access send/return FX |
| **LFO** | LFO assign | Hold + param = assign | Global LFO routing |

### Utility
| Button | Action | Modifier | Description |
|--------|--------|----------|-------------|
| **SHIFT** | Modifier | Hold for alt functions | Access secondary functions |

## Core Workflows

### 1. Write Mode Pattern Entry
```
1. Press [WRITE] (LED turns red)
2. Select track with [T1-T16] buttons  
3. Press step buttons [1-16] to toggle steps
4. Velocity = how hard you press (pressure sensitive)
5. Press [WRITE] again to exit (LED off)
```

**Code Integration**:
```cpp
// Write mode state
bool write_mode = ether_sequencer_is_writing(engine);
ether_sequencer_write_mode(engine, !write_mode);

// Step placement  
if (write_mode && step_pressed) {
    ether_sequencer_toggle_step(engine, current_track, step_index, velocity);
}
```

### 2. Mute/Solo Double-Tap
```
Single Tap Track Button: MUTE track (dim LED)
Double Tap Track Button: SOLO track (bright LED, all others dim)  
Double Tap Solo Track:   UNSOLO (return to normal)
```

**Timing**: Double-tap window = 300ms
**Visual Feedback**: 
- Muted track: LED @ 25% brightness
- Solo track: LED @ 100% brightness  
- Other tracks when solo active: LED @ 10% brightness

**Code Integration**:
```cpp
void onTrackButtonPress(int track, uint32_t timestamp) {
    uint32_t last_press = track_button_times[track];
    bool is_double_tap = (timestamp - last_press) < 300; // 300ms window
    
    if (is_double_tap) {
        ether_sequencer_toggle_solo(engine, track);
    } else {
        ether_sequencer_toggle_mute(engine, track);  
    }
    
    track_button_times[track] = timestamp;
}
```

### 3. Engine Row Selection
```
Hold [ENGINE] + Press [T1-T16]: Assign engine to track
Grid shows current engine per track:
- Row 1-2: Analogue engines (blue LED)
- Row 3-4: FM engines (green LED)  
- Row 5-6: Sample engines (red LED)
- Row 7-16: Other engines (white LED)
```

**Code Integration**:
```cpp
if (engine_button_held && track_pressed) {
    int engine_id = selected_engine; // From grid selection
    ether_sequencer_set_track_engine(engine, track, engine_id);
}
```

### 4. Drum vs Melodic Mode Toggle
```
Hold [SHIFT] + [PATTERN]: Toggle drum/melodic mode
Drum Mode:   Steps trigger samples at fixed pitch
Melodic Mode: Steps trigger notes, pitch varies by row (C3-C5 chromatic)
```

**Mode Indicators**:
- Drum mode: Step LEDs are uniform brightness
- Melodic mode: Step LEDs vary by pitch (brighter = higher pitch)

### 5. OSC Device Connect/Disconnect

#### Connection Flow
```
1. Device sends OSC handshake: "/ethersynth/connect" 
2. EtherSynth responds with: "/ethersynth/ack" + device_id
3. Grid shows connection status (top-right LED green)
4. External controller takes priority over internal sequencer
```

#### Disconnect Handling  
```cpp
void onOSCDisconnect(int device_id) {
    // Graceful fallback to internal sequencer
    osc_connected = false;
    
    // Continue playback without interruption
    if (ether_sequencer_is_playing(engine)) {
        internal_sequencer_resume();
    }
    
    // Visual feedback: connection LED turns red for 2s, then off
    schedule_led_flash(CONNECTION_LED, RED, 2000);
}
```

#### OSC Message Map
```
/step/<track>/<step> <velocity>     - Trigger step
/mute/<track> <state>               - Mute track  
/solo/<track> <state>               - Solo track
/pattern/select <pattern_id>        - Change pattern
/transport/play <state>             - Start/stop
/tempo <bpm>                        - Set BPM
```

## Button Combinations & Shortcuts

### Pattern Operations
| Combination | Action | Description |
|-------------|--------|-------------|
| SHIFT + PLAY | Tap tempo | Set BPM by tapping |
| SHIFT + WRITE | Quantize | Quantize recorded pattern |
| COPY + COPY | Duplicate pattern | Copy to next empty slot |
| DELETE + DELETE | Clear all | Clear current pattern |

### Advanced Navigation
| Combination | Action | Description |
|-------------|--------|-------------|
| SHIFT + Step 1-16 | Pattern select | Quick pattern A1-P16 access |
| ENGINE + FX | Chain mode | Route engine through FX |
| LFO + Step 1-8 | LFO select | Choose global LFO 1-8 |

### Performance Shortcuts
| Combination | Action | Description |
|-------------|--------|-------------|
| SHIFT + Track | Track solo | Instant solo without double-tap |
| PATTERN + Track | Copy track | Copy track to clipboard |
| DELETE + Track | Clear track | Clear track steps |

## Implementation Notes

### Button State Tracking
```cpp
struct ButtonState {
    bool pressed;
    bool held;
    uint32_t press_time;
    uint32_t hold_duration;
};

ButtonState buttons[64]; // All buttons
uint32_t modifier_mask;  // SHIFT, ENGINE, etc.
```

### LED Brightness Control
```cpp
enum LEDBrightness {
    LED_OFF = 0,
    LED_DIM = 64,      // 25% for muted tracks
    LED_NORMAL = 128,   // 50% for active steps  
    LED_BRIGHT = 255    // 100% for solo/selected
};
```

### Timing Requirements
- **Button Response**: <5ms from press to action
- **Double-tap Detection**: 300ms window
- **Visual Feedback**: LED changes within 10ms
- **OSC Latency**: <20ms round-trip for external controllers

This key map provides immediate access to all essential sequencer functions while maintaining intuitive workflows for both live performance and studio programming.