#include "EtherSynthBridge.h"
#include <iostream>

// Minimal mock implementation for testing Swift integration
extern "C" {

// Simple mock struct to represent the synth
struct MockSynth {
    float bpm = 120.0f;
    float masterVolume = 0.8f;
    int activeInstrument = 0;
    bool playing = false;
    bool recording = false;
    float cpuUsage = 15.0f;
    int activeVoices = 0;
};

EtherSynthCpp* ether_create(void) {
    std::cout << "Mock Bridge: Created synth instance" << std::endl;
    return reinterpret_cast<EtherSynthCpp*>(new MockSynth());
}

void ether_destroy(EtherSynthCpp* synth) {
    if (synth) {
        delete reinterpret_cast<MockSynth*>(synth);
        std::cout << "Mock Bridge: Destroyed synth instance" << std::endl;
    }
}

int ether_initialize(EtherSynthCpp* synth) {
    if (!synth) return 0;
    std::cout << "Mock Bridge: Initialized synth" << std::endl;
    return 1; // Success
}

void ether_shutdown(EtherSynthCpp* synth) {
    if (synth) {
        std::cout << "Mock Bridge: Shutdown synth" << std::endl;
    }
}

// Transport controls
void ether_play(EtherSynthCpp* synth) {
    if (synth) {
        MockSynth* mock = reinterpret_cast<MockSynth*>(synth);
        mock->playing = true;
        std::cout << "Mock Bridge: Play" << std::endl;
    }
}

void ether_stop(EtherSynthCpp* synth) {
    if (synth) {
        MockSynth* mock = reinterpret_cast<MockSynth*>(synth);
        mock->playing = false;
        std::cout << "Mock Bridge: Stop" << std::endl;
    }
}

void ether_record(EtherSynthCpp* synth, int enable) {
    if (synth) {
        MockSynth* mock = reinterpret_cast<MockSynth*>(synth);
        mock->recording = (enable != 0);
        std::cout << "Mock Bridge: Record " << (enable ? "ON" : "OFF") << std::endl;
    }
}

int ether_is_playing(EtherSynthCpp* synth) {
    if (synth) {
        MockSynth* mock = reinterpret_cast<MockSynth*>(synth);
        return mock->playing ? 1 : 0;
    }
    return 0;
}

int ether_is_recording(EtherSynthCpp* synth) {
    if (synth) {
        MockSynth* mock = reinterpret_cast<MockSynth*>(synth);
        return mock->recording ? 1 : 0;
    }
    return 0;
}

// Note events
void ether_note_on(EtherSynthCpp* synth, int key_index, float velocity, float aftertouch) {
    if (synth) {
        MockSynth* mock = reinterpret_cast<MockSynth*>(synth);
        mock->activeVoices++;
        std::cout << "Mock Bridge: Note ON " << key_index << " vel=" << velocity << " (voices=" << mock->activeVoices << ")" << std::endl;
    }
}

void ether_note_off(EtherSynthCpp* synth, int key_index) {
    if (synth) {
        MockSynth* mock = reinterpret_cast<MockSynth*>(synth);
        if (mock->activeVoices > 0) mock->activeVoices--;
        std::cout << "Mock Bridge: Note OFF " << key_index << " (voices=" << mock->activeVoices << ")" << std::endl;
    }
}

void ether_all_notes_off(EtherSynthCpp* synth) {
    if (synth) {
        MockSynth* mock = reinterpret_cast<MockSynth*>(synth);
        mock->activeVoices = 0;
        std::cout << "Mock Bridge: All notes OFF" << std::endl;
    }
}

// BPM and timing
void ether_set_bpm(EtherSynthCpp* synth, float bpm) {
    if (synth) {
        MockSynth* mock = reinterpret_cast<MockSynth*>(synth);
        mock->bpm = bpm;
        std::cout << "Mock Bridge: Set BPM " << bpm << std::endl;
    }
}

float ether_get_bpm(EtherSynthCpp* synth) {
    if (synth) {
        MockSynth* mock = reinterpret_cast<MockSynth*>(synth);
        return mock->bpm;
    }
    return 120.0f;
}

// Instrument management
void ether_set_active_instrument(EtherSynthCpp* synth, int color_index) {
    if (synth) {
        MockSynth* mock = reinterpret_cast<MockSynth*>(synth);
        mock->activeInstrument = color_index;
        std::cout << "Mock Bridge: Set active instrument " << color_index << std::endl;
    }
}

int ether_get_active_instrument(EtherSynthCpp* synth) {
    if (synth) {
        MockSynth* mock = reinterpret_cast<MockSynth*>(synth);
        return mock->activeInstrument;
    }
    return 0;
}

// Performance monitoring
float ether_get_cpu_usage(EtherSynthCpp* synth) {
    if (synth) {
        MockSynth* mock = reinterpret_cast<MockSynth*>(synth);
        // Simulate variable CPU usage based on voice count
        mock->cpuUsage = 10.0f + (mock->activeVoices * 3.0f);
        return mock->cpuUsage;
    }
    return 0.0f;
}

int ether_get_active_voice_count(EtherSynthCpp* synth) {
    if (synth) {
        MockSynth* mock = reinterpret_cast<MockSynth*>(synth);
        return mock->activeVoices;
    }
    return 0;
}

float ether_get_master_volume(EtherSynthCpp* synth) {
    if (synth) {
        MockSynth* mock = reinterpret_cast<MockSynth*>(synth);
        return mock->masterVolume;
    }
    return 0.8f;
}

void ether_set_master_volume(EtherSynthCpp* synth, float volume) {
    if (synth) {
        MockSynth* mock = reinterpret_cast<MockSynth*>(synth);
        mock->masterVolume = volume;
        std::cout << "Mock Bridge: Set master volume " << volume << std::endl;
    }
}

// Parameters - simple stub implementations
void ether_set_parameter(EtherSynthCpp* synth, int param_id, float value) {
    std::cout << "Mock Bridge: Set parameter " << param_id << " = " << value << std::endl;
}

float ether_get_parameter(EtherSynthCpp* synth, int param_id) {
    return 0.5f; // Default value
}

void ether_set_instrument_parameter(EtherSynthCpp* synth, int instrument, int param_id, float value) {
    std::cout << "Mock Bridge: Set instrument " << instrument << " param " << param_id << " = " << value << std::endl;
}

float ether_get_instrument_parameter(EtherSynthCpp* synth, int instrument, int param_id) {
    return 0.5f; // Default value
}

// Smart controls - stub implementations
void ether_set_smart_knob(EtherSynthCpp* synth, float value) {
    std::cout << "Mock Bridge: Set smart knob " << value << std::endl;
}

float ether_get_smart_knob(EtherSynthCpp* synth) {
    return 0.5f;
}

void ether_set_touch_position(EtherSynthCpp* synth, float x, float y) {
    std::cout << "Mock Bridge: Set touch position (" << x << ", " << y << ")" << std::endl;
}

} // extern "C"