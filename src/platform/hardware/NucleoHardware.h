#pragma once
#include "HardwareInterface.h"

#ifdef PLATFORM_STM32
#include "stm32f7xx_hal.h" // Adjust for your specific Nucleo board
#include "touchgfx/Application.hpp"
#include "touchgfx/hal/HAL.hpp"
#endif

/**
 * STM32 Nucleo hardware implementation
 * Supports 480x320 touch display and real-time audio processing
 */
class NucleoHardware : public HardwareInterface {
public:
    NucleoHardware();
    ~NucleoHardware() override;
    
    // Core lifecycle
    bool initialize() override;
    void shutdown() override;
    
    // Display management
    void initializeDisplay();
    void updateDisplay();
    void setDisplayBrightness(float brightness) override;
    
    // Touch interface
    TouchPoint getTouch() override;
    bool isTouchActive() override;
    
    // Audio interface (simplified for Nucleo testing)
    bool initializeAudio() override;
    void setAudioCallback(std::function<void(EtherAudioBuffer&)> callback) override;
    
    // Key simulation (for testing without physical keys)
    KeyState getKeyState(uint8_t keyIndex) const override;
    void simulateKeyPress(uint8_t keyIndex, bool pressed);
    
    // Encoders (can be physical or touch-simulated)
    EncoderState getEncoderState(uint8_t encoderIndex) const override;
    void simulateEncoderChange(uint8_t encoderIndex, float delta);
    
    // Smart knob (touch-based simulation)
    float getSmartKnobValue() const override;
    void setSmartKnobValue(float value);
    
    // Hardware info
    const char* getDeviceID() const override { return "STM32 Nucleo"; }
    const char* getFirmwareVersion() const override { return "v1.0-proto"; }
    
    // LEDs and visual feedback
    void setKeyLEDBrightness(uint8_t keyIndex, float brightness) override;
    void setInstrumentLED(InstrumentColor color, float brightness);
    
    // Power management
    void setPowerMode(bool lowPower) override;
    
    // Nucleo-specific features
    void setUserLED(bool on);
    bool getUserButton();
    
    // Display parameters
    static constexpr uint16_t DISPLAY_WIDTH = 480;
    static constexpr uint16_t DISPLAY_HEIGHT = 320;
    static constexpr uint16_t TOUCH_THRESHOLD = 50;
    
private:
    // Hardware state
    bool initialized_ = false;
    bool displayInitialized_ = false;
    bool audioInitialized_ = false;
    
    // Touch state
    TouchPoint currentTouch_;
    bool touchActive_ = false;
    uint32_t lastTouchTime_ = 0;
    
    // Key simulation state (for testing)
    std::array<KeyState, 26> keyStates_;
    
    // Encoder simulation state
    std::array<EncoderState, 4> encoderStates_;
    std::array<float, 4> encoderValues_;
    
    // Smart knob state
    float smartKnobValue_ = 0.5f;
    
    // Audio callback
    std::function<void(EtherAudioBuffer&)> audioCallback_;
    
    // Display buffer (if needed)
    uint16_t* frameBuffer_ = nullptr;
    
    // Hardware initialization helpers
    bool initializeGPIO();
    bool initializeTimers();
    bool initializeTouchController();
    bool initializeAudioHardware();
    
    // Touch processing
    void processTouchInput();
    TouchPoint readTouchController();
    
    // Audio processing (basic implementation)
    void audioInterruptHandler();
    static void audioTimerCallback(void* instance);
    
    // Display helpers
    void clearDisplay();
    void setPixel(uint16_t x, uint16_t y, uint16_t color);
    
    // Hardware-specific constants
    static constexpr uint32_t AUDIO_SAMPLE_RATE = 48000;
    static constexpr uint32_t AUDIO_BUFFER_SIZE = 128;
    static constexpr uint32_t TOUCH_UPDATE_RATE = 100; // 100Hz
};

#ifdef PLATFORM_STM32
// STM32-specific interrupt handlers
extern "C" {
    void DMA2_Stream0_IRQHandler(void);
    void TIM6_DAC_IRQHandler(void);
    void EXTI15_10_IRQHandler(void);
}
#endif