# Normalized steps - always +1 or -1 for grid navigation

import board
import time
import digitalio
import adafruit_seesaw.seesaw
import adafruit_seesaw.rotaryio
import adafruit_seesaw.digitalio

print("Grid Navigation Controller Starting...")

i2c = board.STEMMA_I2C()
seesaw = adafruit_seesaw.seesaw.Seesaw(i2c, 0x49)
print("Connected!")

# Set up encoders and switches
encoders = [adafruit_seesaw.rotaryio.IncrementalEncoder(seesaw, n) for n in range(4)]
switches = [adafruit_seesaw.digitalio.DigitalIO(seesaw, pin) for pin in (12, 14, 17, 9)]
for switch in switches:
    switch.switch_to_input(digitalio.Pull.UP)

print("4 encoders ready for grid navigation")

last_positions = [0, 0, 0, 0]
last_buttons = [True, True, True, True]

print("Ready! Each detent = +1/-1 step")

while True:
    for i in range(4):
        try:
            # Get current encoder position
            current_pos = encoders[i].position

            if current_pos != last_positions[i]:
                # Calculate the actual delta
                raw_delta = current_pos - last_positions[i]

                # Normalize to +1 or -1 based on direction only
                if raw_delta > 0:
                    normalized_delta = 1
                    print(f"E{i+1}:+1")
                else:
                    normalized_delta = -1
                    print(f"E{i+1}:-1")

                last_positions[i] = current_pos

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