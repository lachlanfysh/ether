# Simplified CircuitPython script for QT-PY RP2040 with Adafruit Quad Rotary Encoder
# Based on the existing example in the lib directory

import board
import time
import adafruit_seesaw.seesaw
import adafruit_seesaw.rotaryio
import adafruit_seesaw.digitalio

# Set up I2C - use STEMMA_I2C() which should work on QT-PY
try:
    i2c = board.STEMMA_I2C()
except:
    # Fallback to manual I2C setup
    import busio
    i2c = busio.I2C(board.SCL, board.SDA)

# Initialize seesaw (try common addresses)
seesaw_board = None
addresses = [0x36, 0x49]  # 0x36 is default, 0x49 is alternative

for addr in addresses:
    try:
        print(f"Trying I2C address 0x{addr:02x}")
        seesaw_board = adafruit_seesaw.seesaw.Seesaw(i2c, addr)
        print(f"Found seesaw at address 0x{addr:02x}")
        break
    except Exception as e:
        print(f"Address 0x{addr:02x} failed: {e}")
        continue

if not seesaw_board:
    print("No seesaw device found!")
    while True:
        time.sleep(1)

# Setup encoders - try different pin configurations
encoder_pins = [12, 14, 17, 9]  # Standard pins
button_pins = [24, 25, 26, 27]  # Standard buttons

encoders = []
buttons = []
last_positions = [0, 0, 0, 0]
last_buttons = [False, False, False, False]

print("Setting up encoders...")
for i in range(4):
    try:
        # Setup encoder
        encoder = adafruit_seesaw.rotaryio.IncrementalEncoder(seesaw_board, encoder_pins[i])
        encoders.append(encoder)

        # Setup button
        button = adafruit_seesaw.digitalio.DigitalIO(seesaw_board, button_pins[i])
        button.direction = adafruit_seesaw.digitalio.Direction.INPUT
        button.pull = adafruit_seesaw.digitalio.Pull.UP
        buttons.append(button)

        print(f"Encoder {i+1} initialized successfully")
    except Exception as e:
        print(f"Failed to setup encoder {i+1}: {e}")
        encoders.append(None)
        buttons.append(None)

print("QT-PY Quad Rotary Encoder Controller Ready")

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

    time.sleep(0.01)