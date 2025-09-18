#include "encoder_control_system.h"
#include <iostream>
#include <algorithm>
#include <cmath>

EncoderControlSystem::EncoderControlSystem() {
    // Initialize encoder states
    for (auto& state : encoder_states) {
        state.last_press_time = std::chrono::steady_clock::now();
        state.pending_single_press = false;
    }
}

void EncoderControlSystem::register_parameter(const std::string& id, const std::string& display_name,
                                            float* value_ptr, float min_val, float max_val, float step) {
    parameters.emplace_back(id, display_name, value_ptr, min_val, max_val, step);
    param_id_to_index[id] = parameters.size() - 1;
}

void EncoderControlSystem::handle_encoder_turn(int encoder_id, int delta) {
    if (encoder_id < 1 || encoder_id > 4) return;

    if (encoder_id == 4) {
        // Encoder 4: Menu navigation
        if (menu_state == MenuState::BROWSING) {
            scroll_menu(delta);
        } else if (menu_state == MenuState::EDITING) {
            adjust_current_param(delta * get_current_param().step_size);
        }
    } else {
        // Encoders 1-3: Parameter control
        int enc_index = encoder_id - 1;

        if (!encoder_states[enc_index].latched_params.empty()) {
            // Adjust all latched parameters
            adjust_latched_params(enc_index, delta);
        }
        // If no latches, encoder does nothing (could add default behavior here)
    }
}

void EncoderControlSystem::handle_button_press(int encoder_id) {
    if (encoder_id < 1 || encoder_id > 4) return;

    int enc_index = encoder_id - 1;
    auto now = std::chrono::steady_clock::now();
    auto time_since_last = std::chrono::duration_cast<std::chrono::milliseconds>(
        now - encoder_states[enc_index].last_press_time).count();

    if (time_since_last < DOUBLE_PRESS_TIMEOUT_MS && encoder_states[enc_index].pending_single_press) {
        // Double press detected
        encoder_states[enc_index].pending_single_press = false;

        if (encoder_id == 4) {
            handle_encoder4_press(PressType::DOUBLE);
        } else {
            handle_param_encoder_press(encoder_id, PressType::DOUBLE);
        }
    } else {
        // Potential single press - wait for timeout
        encoder_states[enc_index].pending_single_press = true;
        encoder_states[enc_index].last_press_time = now;
    }
}

void EncoderControlSystem::handle_button_release(int encoder_id) {
    // Button release doesn't trigger actions in this design
    // All logic is on press events
}

void EncoderControlSystem::update() {
    auto now = std::chrono::steady_clock::now();

    for (int i = 0; i < 4; ++i) {
        if (encoder_states[i].pending_single_press) {
            auto time_since_press = std::chrono::duration_cast<std::chrono::milliseconds>(
                now - encoder_states[i].last_press_time).count();

            if (time_since_press >= DOUBLE_PRESS_TIMEOUT_MS) {
                // Single press timeout reached
                encoder_states[i].pending_single_press = false;
                process_pending_single_press(i + 1);
            }
        }
    }
}

void EncoderControlSystem::process_pending_single_press(int encoder_id) {
    if (encoder_id == 4) {
        handle_encoder4_press(PressType::SINGLE);
    } else {
        handle_param_encoder_press(encoder_id, PressType::SINGLE);
    }
}

void EncoderControlSystem::handle_encoder4_press(PressType type) {
    if (type == PressType::SINGLE) {
        if (menu_state == MenuState::BROWSING) {
            // Enter editing mode for current parameter
            menu_state = MenuState::EDITING;
            std::cout << "Entering edit mode for: " << get_current_param_name() << std::endl;
        } else {
            // Exit editing mode, return to browsing
            menu_state = MenuState::BROWSING;
            std::cout << "Exiting edit mode" << std::endl;
        }

        if (menu_callback) {
            menu_callback(get_current_param_id());
        }
    }
    // Double press on encoder 4 could be used for other functions
}

void EncoderControlSystem::handle_param_encoder_press(int encoder_id, PressType type) {
    int enc_index = encoder_id - 1;
    std::string current_param = get_current_param_id();

    if (type == PressType::SINGLE) {
        // Single press: Latch current parameter to this encoder
        auto& latched = encoder_states[enc_index].latched_params;

        // Check if already latched
        auto it = std::find(latched.begin(), latched.end(), current_param);
        if (it == latched.end()) {
            // Add latch
            latched.push_back(current_param);
            std::cout << "Latched " << get_current_param_name() << " to encoder " << encoder_id << std::endl;

            if (latch_callback) {
                latch_callback(encoder_id, current_param, true);
            }
        }
    } else if (type == PressType::DOUBLE) {
        // Double press: Clear all latches for this encoder
        encoder_states[enc_index].latched_params.clear();
        std::cout << "Cleared all latches for encoder " << encoder_id << std::endl;

        if (latch_callback) {
            latch_callback(encoder_id, "", false);  // Signal all cleared
        }
    }
}

void EncoderControlSystem::scroll_menu(int direction) {
    if (parameters.empty()) return;

    // Use navigation callback to update the main UI cursor
    if (nav_callback) {
        nav_callback(direction);
    }

    // Get the new current parameter via callback and update menu
    if (menu_callback && get_current_param_callback) {
        menu_callback(get_current_param_callback());
    }
}

void EncoderControlSystem::adjust_current_param(float delta) {
    // Use callback to adjust the currently selected parameter
    if (get_current_param_callback && adjust_param_callback) {
        std::string current_param_id = get_current_param_callback();
        adjust_param_callback(current_param_id, delta);
    }
}

void EncoderControlSystem::adjust_latched_params(int encoder_index, int delta) {
    for (const std::string& param_id : encoder_states[encoder_index].latched_params) {
        auto it = param_id_to_index.find(param_id);
        if (it != param_id_to_index.end()) {
            Parameter& param = parameters[it->second];
            float change = delta * param.step_size;
            float new_value = *param.value_ptr + change;
            new_value = std::clamp(new_value, param.min_val, param.max_val);

            *param.value_ptr = new_value;

            std::cout << "Encoder " << (encoder_index + 1) << " -> "
                      << param.display_name << ": " << new_value << std::endl;

            if (param_callback) {
                param_callback(param.id, new_value);
            }
        }
    }
}

std::string EncoderControlSystem::get_current_param_id() const {
    if (get_current_param_callback) {
        return get_current_param_callback();
    }
    return "";
}

std::string EncoderControlSystem::get_current_param_name() const {
    std::string param_id = get_current_param_id();
    if (param_id.empty()) return "";

    auto it = param_id_to_index.find(param_id);
    if (it != param_id_to_index.end()) {
        return parameters[it->second].display_name;
    }
    return param_id;
}

const EncoderControlSystem::Parameter& EncoderControlSystem::get_current_param() const {
    std::string param_id = get_current_param_id();
    auto it = param_id_to_index.find(param_id);
    if (it != param_id_to_index.end()) {
        return parameters[it->second];
    }
    // Return first parameter as fallback
    static const Parameter empty_param("", "", nullptr, 0.0f, 1.0f, 0.01f);
    return parameters.empty() ? empty_param : parameters[0];
}

std::vector<std::string> EncoderControlSystem::get_latched_params(int encoder_id) const {
    if (encoder_id < 1 || encoder_id > 3) return {};
    return encoder_states[encoder_id - 1].latched_params;
}

bool EncoderControlSystem::is_param_latched(int encoder_id, const std::string& param_id) const {
    if (encoder_id < 1 || encoder_id > 3) return false;

    const auto& latched = encoder_states[encoder_id - 1].latched_params;
    return std::find(latched.begin(), latched.end(), param_id) != latched.end();
}

void EncoderControlSystem::set_current_param(const std::string& param_id) {
    // No longer needed - cursor is managed externally via callbacks
    (void)param_id; // Suppress unused parameter warning
}

void EncoderControlSystem::clear_all_latches(int encoder_id) {
    if (encoder_id >= 1 && encoder_id <= 3) {
        encoder_states[encoder_id - 1].latched_params.clear();
    }
}