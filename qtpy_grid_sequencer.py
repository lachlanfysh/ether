# Grid sequencer optimized version - single step increments

import board
import time
import digitalio
import adafruit_seesaw.seesaw
import adafruit_seesaw.rotaryio
import adafruit_seesaw.digitalio

print("Grid Sequencer Controller Starting...")

# Use STEMMA_I2C
i2c = board.STEMMA_I2C()
seesaw = adafruit_seesaw.seesaw.Seesaw(i2c, 0x49)
print("Seesaw connected!")

# Set up encoders (pins 0, 1, 2, 3) and switches (pins 12, 14, 17, 9)
encoders = [adafruit_seesaw.rotaryio.IncrementalEncoder(seesaw, n) for n in range(4)]
switches = [adafruit_seesaw.digitalio.DigitalIO(seesaw, pin) for pin in (12, 14, 17, 9)]
for switch in switches:
    switch.switch_to_input(digitalio.Pull.UP)

print("4 encoders and buttons ready")

last_positions = [0, 0, 0, 0]
last_buttons = [True, True, True, True]

# Encoder sensitivity - adjust this to get single steps per detent
ENCODER_DIVISOR = 4  # Try 4 first, adjust if needed

print("Grid Sequencer Ready!")
print("Turn encoders for +1/-1 steps, press for actions")

while True:
    for i in range(4):
        try:
            # Get raw encoder position
            raw_pos = encoders[i].position

            # Convert to single-step increments
            # Divide by ENCODER_DIVISOR to get proper steps
            step_pos = raw_pos // ENCODER_DIVISOR

            if step_pos != last_positions[i]:
                # Send single step increment/decrement
                delta = step_pos - last_positions[i]

                # Clamp large jumps to single steps for grid sequencer
                if delta > 1:
                    delta = 1
                elif delta < -1:
                    delta = -1

                print(f"E{i+1}:{delta:+d}")  # Format with + sign for positive
                last_positions[i] = step_pos

            # Check button press
            current_btn = switches[i].value
            if current_btn != last_buttons[i]:
                if not current_btn:
                    print(f"B{i+1}:PRESS")
                else:
                    print(f"B{i+1}:RELEASE")
                last_buttons[i] = current_btn

        except Exception as e:
            print(f"Error encoder {i+1}: {e}")

    time.sleep(0.01)