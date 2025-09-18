# Fixed script based on Adafruit sample code

import board
import time
import digitalio
import adafruit_seesaw.seesaw
import adafruit_seesaw.rotaryio
import adafruit_seesaw.digitalio

print("Setting up I2C...")

# Use STEMMA_I2C like the sample
i2c = board.STEMMA_I2C()

# Connect to seesaw at address 0x49
print("Connecting to seesaw...")
seesaw = adafruit_seesaw.seesaw.Seesaw(i2c, 0x49)
print("Connected!")

# Set up encoders and switches using the CORRECT pins from sample code
print("Setting up encoders and switches...")

# Encoders use pins 0, 1, 2, 3 (not 12, 14, 17, 9!)
encoders = [adafruit_seesaw.rotaryio.IncrementalEncoder(seesaw, n) for n in range(4)]
print("Encoders setup: pins 0, 1, 2, 3")

# Switches (buttons) use pins 12, 14, 17, 9 (not 24, 25, 26, 27!)
switches = [adafruit_seesaw.digitalio.DigitalIO(seesaw, pin) for pin in (12, 14, 17, 9)]
for switch in switches:
    switch.switch_to_input(digitalio.Pull.UP)  # Use the correct method!
print("Switches setup: pins 12, 14, 17, 9")

# Test each encoder and switch
print("Testing setup...")
for i in range(4):
    try:
        pos = encoders[i].position
        btn = switches[i].value
        print(f"Encoder {i+1}: position={pos}, button={btn}")
    except Exception as e:
        print(f"Encoder {i+1} error: {e}")

last_positions = [0, 0, 0, 0]
last_buttons = [True, True, True, True]  # True = not pressed

print("QT-PY Quad Rotary Encoder Controller Ready!")
print("All encoders and buttons should work now...")

while True:
    for i in range(4):
        try:
            # Check encoder rotation
            current_pos = encoders[i].position
            if current_pos != last_positions[i]:
                delta = current_pos - last_positions[i]
                print(f"E{i+1}:{delta}")
                last_positions[i] = current_pos

            # Check button press
            current_btn = switches[i].value
            if current_btn != last_buttons[i]:
                if not current_btn:  # Button pressed (pull-up inverted)
                    print(f"B{i+1}:PRESS")
                else:  # Button released
                    print(f"B{i+1}:RELEASE")
                last_buttons[i] = current_btn

        except Exception as e:
            print(f"Error reading encoder {i+1}: {e}")

    time.sleep(0.01)