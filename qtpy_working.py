# Working CircuitPython script for QT-PY RP2040 with Adafruit Quad Rotary Encoder
# Fixed imports and address

import board
import time
import adafruit_seesaw.seesaw
import adafruit_seesaw.rotaryio
import adafruit_seesaw.digitalio
from adafruit_seesaw.digitalio import Direction, Pull

# Set up I2C
try:
    i2c = board.STEMMA_I2C()
except:
    import busio
    i2c = busio.I2C(board.SCL, board.SDA)

# Initialize seesaw at address 0x49 (found in previous test)
print("Connecting to seesaw at 0x49")
seesaw_board = adafruit_seesaw.seesaw.Seesaw(i2c, 0x49)
print("Connected!")

# Use the standard Adafruit Quad Rotary Encoder pin configuration
encoder_pins = [12, 14, 17, 9]   # Encoder pins
button_pins = [24, 25, 26, 27]   # Button pins

encoders = []
buttons = []
last_positions = [0, 0, 0, 0]
last_buttons = [False, False, False, False]

print("Setting up encoders...")

# Set up each encoder and button
for i in range(4):
    encoder = None
    button = None

    try:
        # Setup encoder
        encoder = adafruit_seesaw.rotaryio.IncrementalEncoder(seesaw_board, encoder_pins[i])

        # Setup button with correct Direction and Pull imports
        button = adafruit_seesaw.digitalio.DigitalIO(seesaw_board, button_pins[i])
        button.direction = Direction.INPUT
        button.pull = Pull.UP

        print(f"Encoder {i+1} initialized on pins {encoder_pins[i]}/{button_pins[i]}")
    except Exception as e:
        print(f"Failed to setup encoder {i+1}: {e}")

    encoders.append(encoder)
    buttons.append(button)

# Count working encoders
working_count = sum(1 for e in encoders if e is not None)
print(f"Successfully initialized {working_count} encoders")

print("QT-PY Quad Rotary Encoder Controller Ready")
print("Turn encoders or press buttons to see output...")

while True:
    for i in range(4):
        if encoders[i] is None:
            continue

        try:
            # Check encoder rotation
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
            print(f"Error reading encoder {i+1}: {e}")
            # Disable problematic encoder to prevent spam
            if "No such device" in str(e):
                encoders[i] = None
                buttons[i] = None
                print(f"Disabled encoder {i+1}")

    time.sleep(0.01)