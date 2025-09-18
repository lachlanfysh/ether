#ifndef ENCODER_CONTROL_SYSTEM_H
#define ENCODER_CONTROL_SYSTEM_H

#include <chrono>
#include <vector>
#include <string>
#include <functional>
#include <map>

class EncoderControlSystem {
public:
    // Parameter reference structure
    struct Parameter {
        std::string id;           // e.g. "engine2_lpf"
        std::string display_name; // e.g. "Engine 2 LPF"
        float* value_ptr;         // Pointer to actual parameter
        float min_val;
        float max_val;
        float step_size;

        Parameter(const std::string& id, const std::string& name, float* ptr,
                 float min_v, float max_v, float step = 0.01f)
            : id(id), display_name(name), value_ptr(ptr),
              min_val(min_v), max_val(max_v), step_size(step) {}
    };

    // Menu navigation state
    enum class MenuState {
        BROWSING,    // Scrolling through params
        EDITING      // Inside a param, adjusting value
    };

    // Button press types
    enum class PressType {
        SINGLE,
        DOUBLE
    };

    // Callbacks for UI updates
    using MenuUpdateCallback = std::function<void(const std::string& current_param)>;
    using ParameterUpdateCallback = std::function<void(const std::string& param_id, float value)>;
    using LatchUpdateCallback = std::function<void(int encoder, const std::string& param_id, bool latched)>;
    using NavigationCallback = std::function<void(int direction)>;  // direction: +1 or -1
    using GetCurrentParameterCallback = std::function<std::string()>;  // returns current selected parameter ID
    using AdjustParameterCallback = std::function<void(const std::string& param_id, float delta)>;  // adjust parameter by delta

private:
    // Timing constants
    static constexpr int DOUBLE_PRESS_TIMEOUT_MS = 300;

    // Encoder states
    struct EncoderState {
        std::chrono::steady_clock::time_point last_press_time;
        bool pending_single_press = false;
        std::vector<std::string> latched_params;  // Multiple params can be latched to one encoder
    };

    // Menu navigation state
    MenuState menu_state = MenuState::BROWSING;
    // Removed internal cursor - now uses external selectedParamIndex via callbacks

    // Encoder states (0-3, encoder 4 is menu nav)
    std::vector<EncoderState> encoder_states{4};

    // Parameter registry
    std::vector<Parameter> parameters;
    std::map<std::string, size_t> param_id_to_index;

    // Callbacks
    MenuUpdateCallback menu_callback;
    ParameterUpdateCallback param_callback;
    LatchUpdateCallback latch_callback;
    NavigationCallback nav_callback;
    GetCurrentParameterCallback get_current_param_callback;
    AdjustParameterCallback adjust_param_callback;

    // Helper methods
    void process_pending_single_press(int encoder_id);
    void handle_encoder4_press(PressType type);
    void handle_param_encoder_press(int encoder_id, PressType type);
    void scroll_menu(int direction);
    void adjust_current_param(float delta);
    void adjust_latched_params(int encoder_index, int delta);
    std::string get_current_param_id() const;
    const Parameter& get_current_param() const;

public:
    EncoderControlSystem();

    // Setup methods
    void set_menu_callback(MenuUpdateCallback callback) { menu_callback = callback; }
    void set_parameter_callback(ParameterUpdateCallback callback) { param_callback = callback; }
    void set_latch_callback(LatchUpdateCallback callback) { latch_callback = callback; }
    void set_navigation_callback(NavigationCallback callback) { nav_callback = callback; }
    void set_get_current_parameter_callback(GetCurrentParameterCallback callback) { get_current_param_callback = callback; }
    void set_adjust_parameter_callback(AdjustParameterCallback callback) { adjust_param_callback = callback; }

    // Parameter registration
    void register_parameter(const std::string& id, const std::string& display_name,
                          float* value_ptr, float min_val, float max_val, float step = 0.01f);

    // Main input handlers - call these from your serial parser
    void handle_encoder_turn(int encoder_id, int delta);  // +1 or -1
    void handle_button_press(int encoder_id);
    void handle_button_release(int encoder_id);

    // Update method - call periodically to handle timing
    void update();

    // Query methods
    MenuState get_menu_state() const { return menu_state; }
    std::string get_current_param_name() const;
    std::vector<std::string> get_latched_params(int encoder_id) const;
    bool is_param_latched(int encoder_id, const std::string& param_id) const;

    // Control methods
    void set_current_param(const std::string& param_id);
    void clear_all_latches(int encoder_id);
};

#endif // ENCODER_CONTROL_SYSTEM_H