#include "EtherSynthBridge.h"
#include "src/core/EtherSynth.h"
#include "src/audio/AudioEngine.h"
#include <iostream>

// Bridge implementation wrapping our C++ EtherSynth class

extern "C" {

EtherSynthCpp* ether_create(void) {
    try {
        EtherSynth* synth = new EtherSynth();
        std::cout << "C Bridge: Created EtherSynth instance" << std::endl;
        return reinterpret_cast<EtherSynthCpp*>(synth);
    } catch (const std::exception& e) {
        std::cerr << "C Bridge: Failed to create EtherSynth: " << e.what() << std::endl;
        return nullptr;
    }
}

void ether_destroy(EtherSynthCpp* synth) {
    if (synth) {
        EtherSynth* etherSynth = reinterpret_cast<EtherSynth*>(synth);
        delete etherSynth;
        std::cout << "C Bridge: Destroyed EtherSynth instance" << std::endl;
    }
}

int ether_initialize(EtherSynthCpp* synth) {
    if (!synth) return 0;
    
    try {
        EtherSynth* etherSynth = reinterpret_cast<EtherSynth*>(synth);
        bool result = etherSynth->initialize();
        std::cout << "C Bridge: Initialize result: " << (result ? "SUCCESS" : "FAILED") << std::endl;
        return result ? 1 : 0;
    } catch (const std::exception& e) {
        std::cerr << "C Bridge: Initialize error: " << e.what() << std::endl;
        return 0;
    }
}

void ether_shutdown(EtherSynthCpp* synth) {
    if (!synth) return;
    
    try {
        EtherSynth* etherSynth = reinterpret_cast<EtherSynth*>(synth);
        etherSynth->shutdown();
        std::cout << "C Bridge: Shutdown complete" << std::endl;
    } catch (const std::exception& e) {
        std::cerr << "C Bridge: Shutdown error: " << e.what() << std::endl;
    }
}

// Instrument management
void ether_set_active_instrument(EtherSynthCpp* synth, int color_index) {
    if (!synth || color_index < 0 || color_index >= INSTRUMENT_COUNT) return;
    
    try {
        EtherSynth* etherSynth = reinterpret_cast<EtherSynth*>(synth);
        AudioEngine* audioEngine = etherSynth->getAudioEngine();
        if (audioEngine) {
            InstrumentColor color = static_cast<InstrumentColor>(color_index);
            audioEngine->setActiveInstrument(color);
            std::cout << "C Bridge: Set active instrument to " << color_index << std::endl;
        }
    } catch (const std::exception& e) {
        std::cerr << "C Bridge: Set active instrument error: " << e.what() << std::endl;
    }
}

int ether_get_active_instrument(EtherSynthCpp* synth) {
    if (!synth) return 0;
    
    try {
        EtherSynth* etherSynth = reinterpret_cast<EtherSynth*>(synth);
        AudioEngine* audioEngine = etherSynth->getAudioEngine();
        if (audioEngine) {
            return static_cast<int>(audioEngine->getActiveInstrument());
        }
    } catch (const std::exception& e) {
        std::cerr << "C Bridge: Get active instrument error: " << e.what() << std::endl;
    }
    return 0;
}

// Note events
void ether_note_on(EtherSynthCpp* synth, int key_index, float velocity, float aftertouch) {
    if (!synth || key_index < 0 || key_index > 25) return;
    
    try {
        EtherSynth* etherSynth = reinterpret_cast<EtherSynth*>(synth);
        AudioEngine* audioEngine = etherSynth->getAudioEngine();
        if (audioEngine) {
            audioEngine->noteOn(static_cast<uint8_t>(key_index), velocity, aftertouch);
            std::cout << "C Bridge: Note ON " << key_index << " vel=" << velocity << std::endl;
        }
    } catch (const std::exception& e) {
        std::cerr << "C Bridge: Note on error: " << e.what() << std::endl;
    }
}

void ether_note_off(EtherSynthCpp* synth, int key_index) {
    if (!synth || key_index < 0 || key_index > 25) return;
    
    try {
        EtherSynth* etherSynth = reinterpret_cast<EtherSynth*>(synth);
        AudioEngine* audioEngine = etherSynth->getAudioEngine();
        if (audioEngine) {
            audioEngine->noteOff(static_cast<uint8_t>(key_index));
            std::cout << "C Bridge: Note OFF " << key_index << std::endl;
        }
    } catch (const std::exception& e) {
        std::cerr << "C Bridge: Note off error: " << e.what() << std::endl;
    }
}

void ether_all_notes_off(EtherSynthCpp* synth) {
    if (!synth) return;
    
    try {
        EtherSynth* etherSynth = reinterpret_cast<EtherSynth*>(synth);
        AudioEngine* audioEngine = etherSynth->getAudioEngine();
        if (audioEngine) {
            audioEngine->allNotesOff();
            std::cout << "C Bridge: All notes off" << std::endl;
        }
    } catch (const std::exception& e) {
        std::cerr << "C Bridge: All notes off error: " << e.what() << std::endl;
    }
}

// Transport
void ether_play(EtherSynthCpp* synth) {
    if (!synth) return;
    
    try {
        EtherSynth* etherSynth = reinterpret_cast<EtherSynth*>(synth);
        AudioEngine* audioEngine = etherSynth->getAudioEngine();
        if (audioEngine) {
            audioEngine->play();
            std::cout << "C Bridge: Play" << std::endl;
        }
    } catch (const std::exception& e) {
        std::cerr << "C Bridge: Play error: " << e.what() << std::endl;
    }
}

void ether_stop(EtherSynthCpp* synth) {
    if (!synth) return;
    
    try {
        EtherSynth* etherSynth = reinterpret_cast<EtherSynth*>(synth);
        AudioEngine* audioEngine = etherSynth->getAudioEngine();
        if (audioEngine) {
            audioEngine->stop();
            std::cout << "C Bridge: Stop" << std::endl;
        }
    } catch (const std::exception& e) {
        std::cerr << "C Bridge: Stop error: " << e.what() << std::endl;
    }
}

void ether_record(EtherSynthCpp* synth, int enable) {
    if (!synth) return;
    
    try {
        EtherSynth* etherSynth = reinterpret_cast<EtherSynth*>(synth);
        AudioEngine* audioEngine = etherSynth->getAudioEngine();
        if (audioEngine) {
            audioEngine->record(enable != 0);
            std::cout << "C Bridge: Record " << (enable ? "ON" : "OFF") << std::endl;
        }
    } catch (const std::exception& e) {
        std::cerr << "C Bridge: Record error: " << e.what() << std::endl;
    }
}

int ether_is_playing(EtherSynthCpp* synth) {
    if (!synth) return 0;
    
    try {
        EtherSynth* etherSynth = reinterpret_cast<EtherSynth*>(synth);
        AudioEngine* audioEngine = etherSynth->getAudioEngine();
        if (audioEngine) {
            return audioEngine->isPlaying() ? 1 : 0;
        }
    } catch (const std::exception& e) {
        std::cerr << "C Bridge: Is playing error: " << e.what() << std::endl;
    }
    return 0;
}

int ether_is_recording(EtherSynthCpp* synth) {
    if (!synth) return 0;
    
    try {
        EtherSynth* etherSynth = reinterpret_cast<EtherSynth*>(synth);
        AudioEngine* audioEngine = etherSynth->getAudioEngine();
        if (audioEngine) {
            return audioEngine->isRecording() ? 1 : 0;
        }
    } catch (const std::exception& e) {
        std::cerr << "C Bridge: Is recording error: " << e.what() << std::endl;
    }
    return 0;
}

// Parameters
void ether_set_parameter(EtherSynthCpp* synth, int param_id, float value) {
    if (!synth) return;
    
    try {
        EtherSynth* etherSynth = reinterpret_cast<EtherSynth*>(synth);
        AudioEngine* audioEngine = etherSynth->getAudioEngine();
        if (audioEngine) {
            ParameterID param = static_cast<ParameterID>(param_id);
            audioEngine->setParameter(param, value);
            std::cout << "C Bridge: Set parameter " << param_id << " = " << value << std::endl;
        }
    } catch (const std::exception& e) {
        std::cerr << "C Bridge: Set parameter error: " << e.what() << std::endl;
    }
}

float ether_get_parameter(EtherSynthCpp* synth, int param_id) {
    if (!synth) return 0.0f;
    
    try {
        EtherSynth* etherSynth = reinterpret_cast<EtherSynth*>(synth);
        AudioEngine* audioEngine = etherSynth->getAudioEngine();
        if (audioEngine) {
            ParameterID param = static_cast<ParameterID>(param_id);
            return audioEngine->getParameter(param);
        }
    } catch (const std::exception& e) {
        std::cerr << "C Bridge: Get parameter error: " << e.what() << std::endl;
    }
    return 0.0f;
}

void ether_set_instrument_parameter(EtherSynthCpp* synth, int instrument, int param_id, float value) {
    if (!synth) return;
    
    try {
        EtherSynth* etherSynth = reinterpret_cast<EtherSynth*>(synth);
        AudioEngine* audioEngine = etherSynth->getAudioEngine();
        if (audioEngine) {
            InstrumentColor color = static_cast<InstrumentColor>(instrument);
            ParameterID param = static_cast<ParameterID>(param_id);
            audioEngine->setInstrumentParameter(color, param, value);
            std::cout << "C Bridge: Set instrument " << instrument << " param " << param_id << " = " << value << std::endl;
        }
    } catch (const std::exception& e) {
        std::cerr << "C Bridge: Set instrument parameter error: " << e.what() << std::endl;
    }
}

float ether_get_instrument_parameter(EtherSynthCpp* synth, int instrument, int param_id) {
    if (!synth) return 0.0f;
    
    try {
        EtherSynth* etherSynth = reinterpret_cast<EtherSynth*>(synth);
        AudioEngine* audioEngine = etherSynth->getAudioEngine();
        if (audioEngine) {
            InstrumentColor color = static_cast<InstrumentColor>(instrument);
            ParameterID param = static_cast<ParameterID>(param_id);
            return audioEngine->getInstrumentParameter(color, param);
        }
    } catch (const std::exception& e) {
        std::cerr << "C Bridge: Get instrument parameter error: " << e.what() << std::endl;
    }
    return 0.0f;
}

// BPM and timing
void ether_set_bpm(EtherSynthCpp* synth, float bpm) {
    if (!synth) return;
    
    try {
        EtherSynth* etherSynth = reinterpret_cast<EtherSynth*>(synth);
        AudioEngine* audioEngine = etherSynth->getAudioEngine();
        if (audioEngine) {
            audioEngine->setBPM(bpm);
            std::cout << "C Bridge: Set BPM " << bpm << std::endl;
        }
    } catch (const std::exception& e) {
        std::cerr << "C Bridge: Set BPM error: " << e.what() << std::endl;
    }
}

float ether_get_bpm(EtherSynthCpp* synth) {
    if (!synth) return 120.0f;
    
    try {
        EtherSynth* etherSynth = reinterpret_cast<EtherSynth*>(synth);
        AudioEngine* audioEngine = etherSynth->getAudioEngine();
        if (audioEngine) {
            return audioEngine->getBPM();
        }
    } catch (const std::exception& e) {
        std::cerr << "C Bridge: Get BPM error: " << e.what() << std::endl;
    }
    return 120.0f;
}

// Performance metrics
float ether_get_cpu_usage(EtherSynthCpp* synth) {
    if (!synth) return 0.0f;
    
    try {
        EtherSynth* etherSynth = reinterpret_cast<EtherSynth*>(synth);
        AudioEngine* audioEngine = etherSynth->getAudioEngine();
        if (audioEngine) {
            return audioEngine->getCPUUsage();
        }
    } catch (const std::exception& e) {
        std::cerr << "C Bridge: Get CPU usage error: " << e.what() << std::endl;
    }
    return 0.0f;
}

int ether_get_active_voice_count(EtherSynthCpp* synth) {
    if (!synth) return 0;
    
    try {
        EtherSynth* etherSynth = reinterpret_cast<EtherSynth*>(synth);
        AudioEngine* audioEngine = etherSynth->getAudioEngine();
        if (audioEngine) {
            return static_cast<int>(audioEngine->getActiveVoiceCount());
        }
    } catch (const std::exception& e) {
        std::cerr << "C Bridge: Get active voice count error: " << e.what() << std::endl;
    }
    return 0;
}

float ether_get_master_volume(EtherSynthCpp* synth) {
    if (!synth) return 0.8f;
    
    try {
        EtherSynth* etherSynth = reinterpret_cast<EtherSynth*>(synth);
        AudioEngine* audioEngine = etherSynth->getAudioEngine();
        if (audioEngine) {
            return audioEngine->getMasterVolume();
        }
    } catch (const std::exception& e) {
        std::cerr << "C Bridge: Get master volume error: " << e.what() << std::endl;
    }
    return 0.8f;
}

void ether_set_master_volume(EtherSynthCpp* synth, float volume) {
    if (!synth) return;
    
    try {
        EtherSynth* etherSynth = reinterpret_cast<EtherSynth*>(synth);
        AudioEngine* audioEngine = etherSynth->getAudioEngine();
        if (audioEngine) {
            audioEngine->setMasterVolume(volume);
            std::cout << "C Bridge: Set master volume " << volume << std::endl;
        }
    } catch (const std::exception& e) {
        std::cerr << "C Bridge: Set master volume error: " << e.what() << std::endl;
    }
}

// Smart knob and touch - placeholder implementations
void ether_set_smart_knob(EtherSynthCpp* synth, float value) {
    if (!synth) return;
    std::cout << "C Bridge: Set smart knob " << value << std::endl;
    // TODO: Implement smart knob functionality
}

float ether_get_smart_knob(EtherSynthCpp* synth) {
    if (!synth) return 0.5f;
    // TODO: Implement smart knob functionality
    return 0.5f;
}

void ether_set_touch_position(EtherSynthCpp* synth, float x, float y) {
    if (!synth) return;
    std::cout << "C Bridge: Set touch position (" << x << ", " << y << ")" << std::endl;
    // TODO: Implement touch position functionality for morphing
}

} // extern "C"