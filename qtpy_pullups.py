# QT-PY script with manual I2C pull-up configuration

import board
import time
import busio
import digitalio
import adafruit_seesaw.seesaw
import adafruit_seesaw.rotaryio
import adafruit_seesaw.digitalio

print("Setting up I2C with pull-ups...")

# Try to enable pull-ups on I2C pins manually
try:
    sda_pin = digitalio.DigitalInOut(board.SDA)
    scl_pin = digitalio.DigitalInOut(board.SCL)

    sda_pin.direction = digitalio.Direction.INPUT
    scl_pin.direction = digitalio.Direction.INPUT

    sda_pin.pull = digitalio.Pull.UP
    scl_pin.pull = digitalio.Pull.UP

    print("Enabled pull-ups on SDA/SCL")

    # Small delay to let pull-ups stabilize
    time.sleep(0.1)

    # Release pins for I2C use
    sda_pin.deinit()
    scl_pin.deinit()

except Exception as e:
    print(f"Pull-up setup failed: {e}")

# Now set up I2C
try:
    print("Initializing I2C...")
    i2c = busio.I2C(board.SCL, board.SDA)
    print("I2C initialized successfully")
except Exception as e:
    print(f"I2C init failed: {e}")
    print("Trying alternative I2C setup...")

    # Alternative: try STEMMA_I2C if available
    try:
        i2c = board.STEMMA_I2C()
        print("STEMMA I2C initialized")
    except Exception as e2:
        print(f"STEMMA I2C also failed: {e2}")
        print("Continuing anyway...")
        i2c = busio.I2C(board.SCL, board.SDA)

# Scan for I2C devices
print("Scanning for I2C devices...")
try:
    while not i2c.try_lock():
        pass

    devices = i2c.scan()
    i2c.unlock()

    if devices:
        print(f"Found I2C devices at addresses: {[hex(d) for d in devices]}")
    else:
        print("No I2C devices found!")
        print("Check wiring: SDA, SCL, 3V, GND")

except Exception as e:
    print(f"I2C scan failed: {e}")

# Try to connect to seesaw at various addresses
seesaw_board = None
addresses = [0x36, 0x49, 0x3A, 0x3B, 0x60]

for addr in addresses:
    try:
        print(f"Trying seesaw at 0x{addr:02x}...")
        seesaw_board = adafruit_seesaw.seesaw.Seesaw(i2c, addr)
        print(f"Connected to seesaw at 0x{addr:02x}!")
        break
    except Exception as e:
        print(f"  0x{addr:02x} failed: {e}")

if not seesaw_board:
    print("No seesaw device found at any address!")
    print("Make sure the quad rotary encoder board is properly connected")
    while True:
        time.sleep(1)

print("Seesaw connected! Setting up encoders...")

# Rest of the encoder setup code...
encoder_pins = [12, 14, 17, 9]
button_pins = [24, 25, 26, 27]

encoders = []
buttons = []
last_positions = [0, 0, 0, 0]
last_buttons = [False, False, False, False]

for i in range(4):
    encoder = None
    button = None

    try:
        encoder = adafruit_seesaw.rotaryio.IncrementalEncoder(seesaw_board, encoder_pins[i])
        button = adafruit_seesaw.digitalio.DigitalIO(seesaw_board, button_pins[i])
        button.direction = 0  # INPUT
        button.pull = 1       # UP
        print(f"Encoder {i+1}: OK")
    except Exception as e:
        print(f"Encoder {i+1} failed: {e}")

    encoders.append(encoder)
    buttons.append(button)

working = sum(1 for e in encoders if e is not None)
print(f"Working encoders: {working}/4")
print("Ready! Turn encoders or press buttons...")

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
            if "No such device" in str(e):
                encoders[i] = None
                buttons[i] = None

    time.sleep(0.01)