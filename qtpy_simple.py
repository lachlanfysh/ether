# Simple working script for QT-PY with Adafruit Quad Rotary Encoder
# Uses basic constants instead of problematic imports

import board
import time
import busio
import adafruit_seesaw.seesaw
import adafruit_seesaw.rotaryio
import adafruit_seesaw.digitalio

# Set up I2C
i2c = busio.I2C(board.SCL, board.SDA)

# Connect to seesaw at address 0x49 (found working in previous test)
print("Connecting to seesaw at 0x49...")
try:
    seesaw_board = adafruit_seesaw.seesaw.Seesaw(i2c, 0x49)
    print("Connected!")
except Exception as e:
    print(f"Connection failed: {e}")
    while True:
        time.sleep(1)

# Standard Adafruit Quad Rotary Encoder pins
encoder_pins = [12, 14, 17, 9]
button_pins = [24, 25, 26, 27]

encoders = []
buttons = []
last_positions = [0, 0, 0, 0]
last_buttons = [False, False, False, False]

print("Setting up encoders...")

# Set up encoders and buttons
for i in range(4):
    encoder = None
    button = None

    try:
        # Setup encoder
        encoder = adafruit_seesaw.rotaryio.IncrementalEncoder(seesaw_board, encoder_pins[i])
        print(f"Encoder {i+1} setup successful")

        # Setup button - use constants directly instead of imports
        button = adafruit_seesaw.digitalio.DigitalIO(seesaw_board, button_pins[i])
        button.direction = 0  # INPUT = 0
        button.pull = 1       # UP = 1
        print(f"Button {i+1} setup successful")

    except Exception as e:
        print(f"Failed to setup encoder {i+1}: {e}")

    encoders.append(encoder)
    buttons.append(button)

# Count working encoders
working_encoders = sum(1 for e in encoders if e is not None)
working_buttons = sum(1 for b in buttons if b is not None)
print(f"Working: {working_encoders} encoders, {working_buttons} buttons")

print("QT-PY Quad Rotary Encoder Controller Ready!")
print("Turn encoders or press buttons to see output...")

while True:
    for i in range(4):
        try:
            # Check encoder rotation
            if encoders[i] is not None:
                current_pos = encoders[i].position
                if current_pos != last_positions[i]:
                    delta = current_pos - last_positions[i]
                    print(f"E{i+1}:{delta}")
                    last_positions[i] = current_pos

            # Check button press
            if buttons[i] is not None:
                current_btn = buttons[i].value
                if current_btn != last_buttons[i]:
                    if not current_btn:
                        print(f"B{i+1}:PRESS")
                    else:
                        print(f"B{i+1}:RELEASE")
                    last_buttons[i] = current_btn

        except Exception as e:
            if "No such device" in str(e):
                print(f"Disabled encoder {i+1} (no device)")
                encoders[i] = None
                buttons[i] = None
            else:
                print(f"Error encoder {i+1}: {e}")

    time.sleep(0.01)