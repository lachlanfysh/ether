# Debug version to check button functionality

import board
import time
import busio
import digitalio
import adafruit_seesaw.seesaw
import adafruit_seesaw.rotaryio
import adafruit_seesaw.digitalio

print("Setting up I2C...")

# Enable pull-ups manually
try:
    sda_pin = digitalio.DigitalInOut(board.SDA)
    scl_pin = digitalio.DigitalInOut(board.SCL)
    sda_pin.direction = digitalio.Direction.INPUT
    scl_pin.direction = digitalio.Direction.INPUT
    sda_pin.pull = digitalio.Pull.UP
    scl_pin.pull = digitalio.Pull.UP
    time.sleep(0.1)
    sda_pin.deinit()
    scl_pin.deinit()
except:
    pass

i2c = busio.I2C(board.SCL, board.SDA)

# Connect to seesaw
seesaw_board = adafruit_seesaw.seesaw.Seesaw(i2c, 0x49)
print("Connected to seesaw!")

# Focus on encoder 3 since it's the one that works
encoder_pins = [12, 14, 17, 9]
button_pins = [24, 25, 26, 27]

print("Setting up encoder 3 and button 3...")

# Setup encoder 3 (index 2)
encoder3 = adafruit_seesaw.rotaryio.IncrementalEncoder(seesaw_board, encoder_pins[2])
print(f"Encoder 3 setup on pin {encoder_pins[2]}")

# Try different approaches for button 3
button3 = None
button3_pin = button_pins[2]  # Pin 26

print(f"Trying button 3 on pin {button3_pin}...")

try:
    button3 = adafruit_seesaw.digitalio.DigitalIO(seesaw_board, button3_pin)
    button3.direction = 0  # INPUT
    button3.pull = 1       # UP
    print(f"Button 3 setup successful on pin {button3_pin}")

    # Test initial button state
    initial_state = button3.value
    print(f"Button 3 initial state: {initial_state} (True=not pressed, False=pressed)")

except Exception as e:
    print(f"Button 3 setup failed: {e}")

# Also try alternative button pins for encoder 3
alt_button_pins = [23, 24, 25, 27, 28, 29, 30, 31]
alt_button = None

if button3 is None:
    print("Trying alternative button pins for encoder 3...")
    for alt_pin in alt_button_pins:
        try:
            alt_button = adafruit_seesaw.digitalio.DigitalIO(seesaw_board, alt_pin)
            alt_button.direction = 0
            alt_button.pull = 1
            print(f"Alternative button found on pin {alt_pin}")
            button3 = alt_button
            button3_pin = alt_pin
            break
        except Exception as e:
            print(f"Pin {alt_pin} failed: {e}")

last_position = 0
last_button = True if button3 else None

print("Ready! Try encoder 3 rotation and button press...")
print("Monitoring encoder 3 and its button...")

counter = 0
while True:
    counter += 1

    # Check encoder rotation
    current_pos = encoder3.position
    if current_pos != last_position:
        delta = current_pos - last_position
        print(f"E3:{delta}")
        last_position = current_pos

    # Check button - with detailed debugging
    if button3 is not None:
        try:
            current_btn = button3.value

            # Print button state periodically
            if counter % 1000 == 0:  # Every ~10 seconds
                print(f"Button 3 state: {current_btn} (pin {button3_pin})")

            if current_btn != last_button:
                if not current_btn:  # Pressed
                    print(f"B3:PRESS (pin {button3_pin})")
                else:  # Released
                    print(f"B3:RELEASE (pin {button3_pin})")
                last_button = current_btn

        except Exception as e:
            print(f"Button read error: {e}")
            button3 = None

    time.sleep(0.01)