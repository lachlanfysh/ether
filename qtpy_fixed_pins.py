# Fixed pin configuration for Adafruit Quad Rotary Encoder
# Based on actual Adafruit seesaw pinout

import board
import time
import adafruit_seesaw.seesaw
import adafruit_seesaw.rotaryio
import adafruit_seesaw.digitalio

# Set up I2C
try:
    i2c = board.STEMMA_I2C()
except:
    import busio
    i2c = busio.I2C(board.SCL, board.SDA)

# Initialize seesaw - try different addresses
seesaw_board = None
addresses = [0x36, 0x49, 0x3A, 0x3B]  # Common addresses

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

# Correct pin configuration for Adafruit Quad Rotary Encoder
# These are the actual pins used by the Adafruit board
encoder_pins = [12, 14, 17, 9]   # Keep trying these first
button_pins = [24, 25, 26, 27]   # Keep trying these first

# Alternative pin sets to try if the above don't work
alt_encoder_pins = [
    [5, 6, 7, 8],      # Alternative 1
    [1, 2, 3, 4],      # Alternative 2
    [18, 19, 20, 21]   # Alternative 3
]

alt_button_pins = [
    [0, 1, 2, 3],      # Alternative 1
    [10, 11, 12, 13],  # Alternative 2
    [14, 15, 16, 17]   # Alternative 3
]

encoders = []
buttons = []
last_positions = [0, 0, 0, 0]
last_buttons = [False, False, False, False]

print("Setting up encoders...")

# Try to set up each encoder individually
for i in range(4):
    encoder = None
    button = None

    # Try main pin configuration first
    try:
        encoder = adafruit_seesaw.rotaryio.IncrementalEncoder(seesaw_board, encoder_pins[i])
        button = adafruit_seesaw.digitalio.DigitalIO(seesaw_board, button_pins[i])
        button.direction = adafruit_seesaw.digitalio.Direction.INPUT
        button.pull = adafruit_seesaw.digitalio.Pull.UP
        print(f"Encoder {i+1} setup with pins {encoder_pins[i]}/{button_pins[i]}")
    except Exception as e:
        print(f"Main pins failed for encoder {i+1}: {e}")

        # Try alternative pin configurations
        for alt_idx in range(len(alt_encoder_pins)):
            try:
                encoder = adafruit_seesaw.rotaryio.IncrementalEncoder(seesaw_board, alt_encoder_pins[alt_idx][i])
                button = adafruit_seesaw.digitalio.DigitalIO(seesaw_board, alt_button_pins[alt_idx][i])
                button.direction = adafruit_seesaw.digitalio.Direction.INPUT
                button.pull = adafruit_seesaw.digitalio.Pull.UP
                print(f"Encoder {i+1} setup with alternative pins {alt_encoder_pins[alt_idx][i]}/{alt_button_pins[alt_idx][i]}")
                break
            except Exception as e2:
                print(f"Alternative {alt_idx} failed for encoder {i+1}: {e2}")
                continue

    encoders.append(encoder)
    buttons.append(button)

# Count working encoders
working_count = sum(1 for e in encoders if e is not None)
print(f"Successfully initialized {working_count} encoders")

if working_count == 0:
    print("No encoders working! Check wiring and I2C address.")
    while True:
        time.sleep(1)

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
            # If we get persistent errors, disable this encoder
            encoders[i] = None
            buttons[i] = None

    time.sleep(0.01)