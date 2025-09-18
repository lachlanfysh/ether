# CircuitPython script for QT-PY RP2040 with 4 push encoders
# This replaces MIDI with direct serial communication for full encoder resolution

import board
import rotaryio
import digitalio
import time
import usb_cdc

# Setup 4 encoders - adjust pins based on your wiring
# Encoder 1
enc1 = rotaryio.IncrementalEncoder(board.A0, board.A1)
btn1 = digitalio.DigitalInOut(board.A2)
btn1.direction = digitalio.Direction.INPUT
btn1.pull = digitalio.Pull.UP

# Encoder 2  
enc2 = rotaryio.IncrementalEncoder(board.A3, board.TX)
btn2 = digitalio.DigitalInOut(board.RX)
btn2.direction = digitalio.Direction.INPUT
btn2.pull = digitalio.Pull.UP

# Encoder 3
enc3 = rotaryio.IncrementalEncoder(board.SCL, board.SDA)
btn3 = digitalio.DigitalInOut(board.MISO)
btn3.direction = digitalio.Direction.INPUT
btn3.pull = digitalio.Pull.UP

# Encoder 4
enc4 = rotaryio.IncrementalEncoder(board.MOSI, board.SCK)
btn4 = digitalio.DigitalInOut(board.NEOPIXEL)
btn4.direction = digitalio.Direction.INPUT
btn4.pull = digitalio.Pull.UP

# Track previous values
last_positions = [0, 0, 0, 0]
last_buttons = [True, True, True, True]  # True = not pressed (pull-up)

encoders = [enc1, enc2, enc3, enc4]
buttons = [btn1, btn2, btn3, btn4]

print("QT-PY Encoder Controller Ready")

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