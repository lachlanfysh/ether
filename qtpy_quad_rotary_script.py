# CircuitPython script for QT-PY RP2040 with Adafruit Quad Rotary Encoder board
# Uses I2C communication for full encoder resolution

import board
import busio
import time
from adafruit_seesaw import seesaw, rotaryio, digitalio

# Setup I2C
i2c = busio.I2C(board.SCL, board.SDA)

# Initialize the quad rotary encoder board
# Default I2C address is 0x36, but can be changed with jumpers
seesaw_board = seesaw.Seesaw(i2c, addr=0x36)

# Setup 4 encoders and buttons
encoders = []
buttons = []
last_positions = [0, 0, 0, 0]
last_buttons = [False, False, False, False]

# Initialize encoders and buttons (pins 12, 14, 17, 9 for encoders, 24, 25, 26, 27 for buttons)
encoder_pins = [12, 14, 17, 9]
button_pins = [24, 25, 26, 27]

for i in range(4):
    # Setup encoder
    encoder = rotaryio.IncrementalEncoder(seesaw_board, encoder_pins[i])
    encoders.append(encoder)

    # Setup button
    button = digitalio.DigitalIO(seesaw_board, button_pins[i])
    button.direction = digitalio.Direction.INPUT
    button.pull = digitalio.Pull.UP
    buttons.append(button)

print("QT-PY Quad Rotary Encoder Controller Ready")

while True:
    # Check each encoder
    for i in range(4):
        # Check encoder rotation
        current_pos = encoders[i].position
        if current_pos != last_positions[i]:
            delta = current_pos - last_positions[i]
            print(f"E{i+1}:{delta}")  # E1:+2 or E1:-1 etc
            last_positions[i] = current_pos

        # Check button press
        current_btn = buttons[i].value
        if current_btn != last_buttons[i]:
            if not current_btn:  # Button pressed (pull-up inverted)
                print(f"B{i+1}:PRESS")
            else:  # Button released
                print(f"B{i+1}:RELEASE")
            last_buttons[i] = current_btn

    time.sleep(0.01)  # 10ms polling